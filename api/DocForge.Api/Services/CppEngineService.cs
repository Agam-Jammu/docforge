using System.Diagnostics;
using System.Text.Json;

namespace DocForge.Api.Services;

/// <summary>
/// Wrapper around the C++ DocForge CLI binary for OCR + field extraction.
/// The native P/Invoke path is preserved but falls back to shelling out
/// to the compiled docforge_cli binary which works on macOS ARM64.
/// </summary>
public class CppEngineService : IDisposable
{
    private readonly ILogger<CppEngineService> _logger;
    private readonly string? _cliPath;

    public CppEngineService(ILogger<CppEngineService> logger)
    {
        _logger = logger;

        // Look for the CLI binary:
        // 1. Docker deployment — bundled alongside the API binary
        // 2. Local dev — compiled in the cpp-engine/build directory
        // 3. Fallback — try the PATH
        var possiblePaths = new[]
        {
            Path.Combine(AppContext.BaseDirectory, "docforge_cli"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "..", "cpp-engine", "build", "docforge_cli"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "cpp-engine", "build", "docforge_cli"),
            Path.Combine(Directory.GetCurrentDirectory(), "..", "..", "cpp-engine", "build", "docforge_cli"),
            "/Users/agamdeep/personal/projects/docforge/cpp-engine/build/docforge_cli",
            "docforge_cli",
        };

        _cliPath = null!;
        foreach (var path in possiblePaths)
        {
            var full = Path.GetFullPath(path);
            if (File.Exists(full))
            {
                _cliPath = full;
                _logger.LogInformation("Found C++ engine CLI at {Path}", _cliPath);
                break;
            }
        }

        if (_cliPath == null)
        {
            _logger.LogWarning("C++ engine CLI not found — searched paths: {Paths}",
                string.Join(", ", possiblePaths));
        }
    }

    public string? ProcessDocument(string filePath, string documentType)
    {
        if (_cliPath == null)
        {
            _logger.LogError("Cannot process document {File}: C++ CLI binary not found", filePath);
            return null;
        }

        if (!File.Exists(filePath))
        {
            _logger.LogError("Cannot process document {File}: file does not exist", filePath);
            return null;
        }

        return ProcessViaCli(filePath, documentType);
    }

    private string? ProcessViaCli(string filePath, string documentType)
    {
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = _cliPath,
                Arguments = $"--type {documentType} \"{filePath}\"",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };

            using var process = new Process { StartInfo = psi };
            process.Start();

            // Read stdout and stderr concurrently to avoid deadlocks
            var stdoutTask = process.StandardOutput.ReadToEndAsync();
            var stderrTask = process.StandardError.ReadToEndAsync();

            if (!process.WaitForExit(TimeSpan.FromSeconds(60)))
            {
                _logger.LogWarning("C++ engine CLI timed out after 60s for {File} (type: {Type})", filePath, documentType);
                process.Kill();
                return null;
            }

            var stdout = stdoutTask.Result;
            var stderr = stderrTask.Result;

            if (process.ExitCode != 0)
            {
                _logger.LogWarning("C++ engine CLI exited with code {Code} for {File} (type: {Type}). Stderr: {Stderr}",
                    process.ExitCode, filePath, documentType, stderr);
                return null;
            }

            if (!string.IsNullOrEmpty(stderr))
            {
                _logger.LogInformation("C++ engine CLI stderr for {File}: {Stderr}", filePath, stderr);
            }

            // Extract the JSON object from the output.
            // The CLI wraps results in an array: [ { ... JSON ... } ]
            var jsonStart = stdout.IndexOf('{');
            var jsonEnd = stdout.LastIndexOf('}');
            if (jsonStart >= 0 && jsonEnd > jsonStart)
            {
                var json = stdout.Substring(jsonStart, jsonEnd - jsonStart + 1);

                // The C++ engine may emit unescaped newlines in field values (e.g., line_items).
                // Replace literal newlines with \n escape sequences so JsonSerializer can parse it.
                json = json.Replace("\r\n", "\\n").Replace("\r", "\\n").Replace("\n", "\\n");

                _logger.LogInformation("C++ engine CLI: extracted result for {File} (type: {Type})", filePath, documentType);
                return json;
            }

            _logger.LogWarning("Could not parse CLI output for {File} (type: {Type}). Stdout: {Stdout}",
                filePath, documentType, stdout);
            return null;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to run C++ engine CLI for {File} (type: {Type})", filePath, documentType);
            return null;
        }
    }

    public void Dispose()
    {
        // No native resources to release
    }
}