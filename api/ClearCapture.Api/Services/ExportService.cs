using System.Net.Http.Json;
using System.Text;
using System.Text.Json;
using ClearCapture.Api.Data;
using ClearCapture.Api.Models;
using Microsoft.EntityFrameworkCore;

namespace ClearCapture.Api.Services;

/// <summary>
/// Handles delivery of validated documents to configured export targets.
/// 
/// Three export targets (mirroring Core Capture's "export options"):
///   1. json_webhook  — POST structured metadata to a configurable endpoint
///   2. postgres_write — Insert into a normalized mock ERP/CRM table
///   3. file_export   — Write a metadata sidecar JSON file alongside the document
/// </summary>
public class ExportService
{
    private readonly IServiceScopeFactory _scopeFactory;
    private readonly HttpClient _http;
    private readonly ILogger<ExportService> _logger;

    public ExportService(
        IServiceScopeFactory scopeFactory,
        HttpClient http,
        ILogger<ExportService> logger)
    {
        _scopeFactory = scopeFactory;
        _http = http;
        _logger = logger;
    }

    public async Task<ExportResult> ExportAsync(Guid documentId)
    {
        using var scope = _scopeFactory.CreateScope();
        var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();

        var doc = await db.Documents
            .Include(d => d.ExtractedFields)
            .FirstOrDefaultAsync(d => d.Id == documentId);

        if (doc == null)
            return new ExportResult { Success = false, Error = "Document not found" };

        if (doc.Status != DocumentStatus.Validated)
            return new ExportResult { Success = false, Error = "Document must be validated before export" };

        var workflow = await db.WorkflowConfigs
            .FirstOrDefaultAsync(w => w.DocumentType == doc.DocumentType);

        var destination = workflow?.ExportTarget ?? "json_webhook";
        var configJson = workflow?.ExportConfigJson ?? "{}";

        try
        {
            var payload = BuildExportPayload(doc);
            string responseJson;

            switch (destination)
            {
                case "json_webhook":
                    responseJson = await ExportToWebhook(payload, configJson);
                    break;
                case "postgres_write":
                    responseJson = await ExportToPostgres(db, doc, payload);
                    break;
                case "file_export":
                    responseJson = await ExportToFile(doc, payload, configJson);
                    break;
                default:
                    responseJson = $"{{\"error\":\"Unknown export target: {destination}\"}}";
                    break;
            }

            db.ExportLogs.Add(new ExportLog
            {
                DocumentId = documentId,
                Destination = destination,
                Status = "exported",
                ResponseJson = responseJson,
            });

            doc.Status = DocumentStatus.Exported;
            await db.SaveChangesAsync();

            return new ExportResult
            {
                Success = true,
                Destination = destination,
                ResponseJson = responseJson,
            };
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Export failed for document {Id} to {Dest}", documentId, destination);

            db.ExportLogs.Add(new ExportLog
            {
                DocumentId = documentId,
                Destination = destination,
                Status = "failed",
                ResponseJson = $"{{\"error\":\"{ex.Message}\"}}",
            });
            await db.SaveChangesAsync();

            return new ExportResult
            {
                Success = false,
                Error = ex.Message,
                Destination = destination,
            };
        }
    }

    private static ExportPayload BuildExportPayload(Document doc)
    {
        return new ExportPayload
        {
            DocumentId = doc.Id,
            Filename = doc.Filename,
            DocumentType = doc.DocumentType,
            Confidence = doc.Confidence,
            ProcessedAt = DateTime.UtcNow,
            Fields = doc.ExtractedFields.Select(f => new ExportField
            {
                FieldName = f.FieldName,
                Value = f.CorrectedValue ?? f.ExtractedValue,
                Confidence = f.Confidence,
                IsHumanCorrected = f.IsHumanCorrected,
            }).ToList(),
        };
    }

    private async Task<string> ExportToWebhook(ExportPayload payload, string configJson)
    {
        ExportConfig? config;
        try { config = JsonSerializer.Deserialize<ExportConfig>(configJson); }
        catch { config = null; }

        var url = config?.Url ?? "http://localhost:9090/webhook";
        _logger.LogInformation("Exporting to webhook: {Url}", url);

        var response = await _http.PostAsJsonAsync(url, payload);
        response.EnsureSuccessStatusCode();
        var body = await response.Content.ReadAsStringAsync();
        return $"{{\"webhook_status\":{(int)response.StatusCode},\"response\":\"{body}\"}}";
    }

    private async Task<string> ExportToPostgres(AppDbContext db, Document doc, ExportPayload payload)
    {
        _logger.LogInformation("Exporting to PostgreSQL mock ERP: {Id}", doc.Id);

        // Insert into a mock ERP orders table
        db.MockErpOrders.Add(new MockErpOrder
        {
            DocumentId = doc.Id,
            DocumentType = doc.DocumentType,
            Filename = doc.Filename,
            PayloadJson = JsonSerializer.Serialize(payload),
            CreatedAt = DateTime.UtcNow,
        });
        await db.SaveChangesAsync();

        return $"{{\"table\":\"mock_erp_orders\",\"document_id\":\"{doc.Id}\"}}";
    }

    private async Task<string> ExportToFile(Document doc, ExportPayload payload, string configJson)
    {
        ExportConfig? config;
        try { config = JsonSerializer.Deserialize<ExportConfig>(configJson); }
        catch { config = null; }

        var outputDir = config?.OutputDir ?? "/exports";
        Directory.CreateDirectory(outputDir);

        var outputPath = Path.Combine(outputDir, $"{doc.Id}.metadata.json");
        var json = JsonSerializer.Serialize(payload, new JsonSerializerOptions { WriteIndented = true });
        await File.WriteAllTextAsync(outputPath, json, Encoding.UTF8);

        _logger.LogInformation("Exported metadata to file: {Path}", outputPath);
        return $"{{\"file\":\"{outputPath}\",\"size\":{json.Length}}}";
    }
}

public class ExportResult
{
    public bool Success { get; set; }
    public string? Error { get; set; }
    public string? Destination { get; set; }
    public string? ResponseJson { get; set; }
}

public class ExportPayload
{
    public Guid DocumentId { get; set; }
    public string Filename { get; set; } = "";
    public string DocumentType { get; set; } = "";
    public double Confidence { get; set; }
    public DateTime ProcessedAt { get; set; }
    public List<ExportField> Fields { get; set; } = new();
}

public class ExportField
{
    public string FieldName { get; set; } = "";
    public string Value { get; set; } = "";
    public double Confidence { get; set; }
    public bool IsHumanCorrected { get; set; }
}

public class ExportConfig
{
    public string? Url { get; set; }
    public string? OutputDir { get; set; }
}