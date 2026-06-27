using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace ClearCapture.Api.Services;

/// <summary>
/// P/Invoke wrapper around the C++ ClearCapture engine shared library.
/// Falls back to shelling out to clearcapture_cli if the native lib isn't loadable.
/// </summary>
public class CppEngineService : IDisposable
{
    private const string DllPath = "libclearcapture_engine";
    private readonly bool _nativeAvailable;
    private readonly string? _cliPath;
    private readonly ILogger<CppEngineService> _logger;

    [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr CLEAR_Initialize(int numThreads);

    [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr CLEAR_ProcessDocument(string filePath, string documentType);

    [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
    private static extern void CLEAR_FreeString(IntPtr ptr);

    [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
    private static extern void CLEAR_Shutdown();

    public CppEngineService(ILogger<CppEngineService> logger)
    {
        _logger = logger;

        // Try native P/Invoke first
        try
        {
            var result = CLEAR_Initialize(Environment.ProcessorCount);
            if (result != IntPtr.Zero)
            {
                var msg = Marshal.PtrToStringAnsi(result) ?? "Unknown error";
                CLEAR_FreeString(result);
                _logger.LogInformation("C++ engine initialized: {Msg}", msg);
            }
            _nativeAvailable = true;
            _logger.LogInformation("Using native P/Invoke for C++ engine");
            return;
        }
        catch (DllNotFoundException ex)
        {
            _logger.LogWarning("Native C++ engine not found ({Ex}), falling back to CLI", ex.Message);
        }
        catch (Exception ex)
        {
            _logger.LogWarning("Failed to load native C++ engine ({Ex}), falling back to CLI", ex.Message);
        }

        _nativeAvailable = false;

        // Look for clearcapture_cli in common locations
        var candidates = new[]
        {
            Path.Combine(Directory.GetCurrentDirectory(), "..", "..", "cpp-engine", "build", "clearcapture_cli"),
            Path.Combine(Directory.GetCurrentDirectory(), "clearcapture_cli"),
            "/usr/local/bin/clearcapture_cli",
            "../cpp-engine/build/clearcapture_cli"
        };

        foreach (var c in candidates)
        {
            var full = Path.GetFullPath(c);
            if (File.Exists(full))
            {
                _cliPath = full;
                _logger.LogInformation("Found CLI fallback at {Path}", full);
                return;
            }
        }

        _logger.LogWarning("No C++ engine available — documents will be processed in mock mode");
    }

    public string? ProcessDocument(string filePath, string documentType)
    {
        if (_nativeAvailable)
        {
            return ProcessNative(filePath, documentType);
        }
        return ProcessCli(filePath, documentType);
    }

    private string? ProcessNative(string filePath, string documentType)
    {
        var ptr = CLEAR_ProcessDocument(filePath, documentType);
        if (ptr == IntPtr.Zero) return null;
        var json = Marshal.PtrToStringAnsi(ptr);
        CLEAR_FreeString(ptr);
        return json;
    }

    private string? ProcessCli(string filePath, string documentType)
    {
        if (_cliPath == null)
        {
            // Mock result for when no engine is available
            return JsonSerializer.Serialize(new
            {
                filename = Path.GetFileName(filePath),
                document_type = documentType,
                confidence = 85.0,
                page_count = 1,
                fields = new[]
                {
                    new { field_name = "vendor_name", value = "Mock Corp", confidence = 85.0, bounding_box = new { x = 30, y = 70, width = 200, height = 20 } },
                    new { field_name = "total_amount", value = "165.00", confidence = 85.0, bounding_box = new { x = 30, y = 360, width = 120, height = 20 } },
                    new { field_name = "date", value = "2026-06-15", confidence = 85.0, bounding_box = new { x = 80, y = 150, width = 140, height = 20 } }
                },
                raw_text = "Mock OCR text from fallback engine"
            });
        }

        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = _cliPath,
                Arguments = $"\"{filePath}\"",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = Process.Start(psi);
            if (process == null)
            {
                _logger.LogError("Failed to start CLI process");
                return null;
            }

            var output = process.StandardOutput.ReadToEnd();
            process.WaitForExit(30000);

            if (process.ExitCode != 0)
            {
                var err = process.StandardError.ReadToEnd();
                _logger.LogError("CLI exited with code {Code}: {Err}", process.ExitCode, err);
                return null;
            }

            // CLI mixes log lines with JSON. The JSON starts with '{"' (object fields)
            try
            {
                var objStart = output.IndexOf("{\"", StringComparison.Ordinal);
                if (objStart >= 0)
                {
                    // Parse as JSON array: "[{...}, ...]"
                    var arrayStart = output.LastIndexOf('[', objStart);
                    if (arrayStart < 0) arrayStart = objStart - 1;
                    var arrayEnd = output.LastIndexOf(']');
                    if (arrayEnd > objStart)
                    {
                        var jsonArray = arrayStart >= 0 ? output[arrayStart..(arrayEnd + 1)] : output[objStart..(arrayEnd + 1)];
                        using var doc = JsonDocument.Parse(jsonArray);
                        if (doc.RootElement.ValueKind == JsonValueKind.Array && doc.RootElement.GetArrayLength() > 0)
                        {
                            var result = doc.RootElement[0].GetRawText();
                            _logger.LogInformation("CLI returned result for {File}", filePath);
                            return result;
                        }
                    }
                }
            }
            catch (JsonException ex)
            {
                _logger.LogError(ex, "Failed to parse CLI JSON for {File}");
            }

            return null;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error running CLI for {File}", filePath);
            return null;
        }
    }

    public void Dispose()
    {
        if (_nativeAvailable)
        {
            try { CLEAR_Shutdown(); } catch { }
        }
    }
}