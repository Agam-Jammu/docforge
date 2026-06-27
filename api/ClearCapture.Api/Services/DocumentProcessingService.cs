using ClearCapture.Api.Data;
using ClearCapture.Api.Models;
using Microsoft.EntityFrameworkCore;
using System.Text.Json;

namespace ClearCapture.Api.Services;

/// <summary>
/// Background service that polls for pending documents and processes them
/// through the C++ engine. This is the IHostedService equivalent of a Windows Service.
/// </summary>
public class DocumentProcessingService : BackgroundService
{
    private readonly IServiceScopeFactory _scopeFactory;
    private readonly ILogger<DocumentProcessingService> _logger;

    public DocumentProcessingService(
        IServiceScopeFactory scopeFactory,
        ILogger<DocumentProcessingService> logger)
    {
        _scopeFactory = scopeFactory;
        _logger = logger;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        _logger.LogInformation("DocumentProcessingService started");

        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                await ProcessNextBatch(stoppingToken);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error in document processing loop");
            }

            await Task.Delay(TimeSpan.FromSeconds(2), stoppingToken);
        }
    }

    private async Task ProcessNextBatch(CancellationToken ct)
    {
        using var scope = _scopeFactory.CreateScope();
        var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
        var engine = scope.ServiceProvider.GetRequiredService<CppEngineService>();

        var pending = await db.Documents
            .Where(d => d.Status == DocumentStatus.Pending)
            .OrderBy(d => d.UploadedAt)
            .Take(5)
            .ToListAsync(ct);

        foreach (var doc in pending)
        {
            if (ct.IsCancellationRequested) break;

            try
            {
                doc.Status = DocumentStatus.Processing;
                await db.SaveChangesAsync(ct);

                _logger.LogInformation("Processing document {Id}: {Filename}", doc.Id, doc.Filename);

                var json = engine.ProcessDocument(doc.RawFilePath!, doc.DocumentType);

                if (json != null)
                {
                    var result = JsonSerializer.Deserialize<EngineResult>(json);
                    if (result != null)
                    {
                        doc.DocumentType = result.DocumentType ?? doc.DocumentType;
                        doc.Confidence = result.Confidence;
                        doc.RawText = result.RawText;
                        doc.Status = DocumentStatus.Validated;

                        if (result.Fields != null)
                        {
                            foreach (var field in result.Fields)
                            {
                                db.ExtractedFields.Add(new ExtractedField
                                {
                                    DocumentId = doc.Id,
                                    FieldName = field.FieldName ?? "unknown",
                                    ExtractedValue = field.Value ?? string.Empty,
                                    Confidence = field.Confidence,
                                    BoundingBoxJson = JsonSerializer.Serialize(field.BoundingBox)
                                });
                            }
                        }
                    }
                }
                else
                {
                    doc.Status = DocumentStatus.Failed;
                }

                await db.SaveChangesAsync(ct);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Failed to process document {Id}", doc.Id);
                doc.Status = DocumentStatus.Failed;
                await db.SaveChangesAsync(ct);
            }
        }
    }

    private class EngineResult
    {
        public string? Filename { get; set; }
        public string? DocumentType { get; set; }
        public double Confidence { get; set; }
        public string? RawText { get; set; }
        public List<EngineField>? Fields { get; set; }
    }

    private class EngineField
    {
        public string? FieldName { get; set; }
        public string? Value { get; set; }
        public double Confidence { get; set; }
        public BoundingBox? BoundingBox { get; set; }
    }

    private class BoundingBox
    {
        public int X { get; set; }
        public int Y { get; set; }
        public int Width { get; set; }
        public int Height { get; set; }
    }
}