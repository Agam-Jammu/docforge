using DocForge.Api.Data;
using DocForge.Api.Models;
using DocForge.Api.Services;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace DocForge.Api.Controllers;

[ApiController]
[Route("api/[controller]")]
public class DocumentsController : ControllerBase
{
    private readonly AppDbContext _db;
    private readonly ILogger<DocumentsController> _logger;

    public DocumentsController(AppDbContext db, ILogger<DocumentsController> logger)
    {
        _db = db;
        _logger = logger;
    }

    /// <summary>
    /// Get all documents, ordered by most recent upload.
    /// </summary>
    [HttpGet]
    public async Task<IActionResult> GetAll()
    {
        var docs = await _db.Documents
            .OrderByDescending(d => d.UploadedAt)
            .Select(d => new
            {
                d.Id,
                d.Filename,
                d.DocumentType,
                Status = d.Status.ToString(),
                d.Confidence,
                d.UploadedAt
            })
            .ToListAsync();

        return Ok(docs);
    }

    /// <summary>
    /// Upload one or more documents for processing.
    /// </summary>
    [HttpPost]
    public async Task<IActionResult> Upload([FromForm] List<IFormFile> files)
    {
        if (files == null || files.Count == 0)
            return BadRequest("No files uploaded");

        var uploadsDir = Path.Combine(Directory.GetCurrentDirectory(), "Uploads");
        Directory.CreateDirectory(uploadsDir);

        var documents = new List<Document>();

        foreach (var file in files)
        {
            var filePath = Path.Combine(uploadsDir, $"{Guid.NewGuid()}_{file.FileName}");
            using (var stream = new FileStream(filePath, FileMode.Create))
            {
                await file.CopyToAsync(stream);
            }

            var doc = new Document
            {
                Filename = file.FileName,
                RawFilePath = filePath,
                Status = DocumentStatus.Pending
            };
            _db.Documents.Add(doc);
            documents.Add(doc);
        }

        await _db.SaveChangesAsync();

        return Ok(documents.Select(d => new
        {
            d.Id, d.Filename, Status = d.Status.ToString(), d.UploadedAt
        }));
    }

    /// <summary>
    /// Get all documents (alias for GET /api/documents with status filtering).
    /// </summary>
    [HttpGet("status")]
    public async Task<IActionResult> GetAllByStatus([FromQuery] string? status)
    {
        var query = _db.Documents.AsQueryable();
        if (!string.IsNullOrWhiteSpace(status))
        {
            query = query.Where(d => d.Status.ToString() == status);
        }

        var docs = await query
            .OrderByDescending(d => d.UploadedAt)
            .Select(d => new
            {
                d.Id,
                d.Filename,
                d.DocumentType,
                Status = d.Status.ToString(),
                d.Confidence,
                d.UploadedAt
            })
            .ToListAsync();

        return Ok(docs);
    }

    /// <summary>
    /// Get the processing status of a document.
    /// </summary>
    [HttpGet("{id:guid}/status")]
    public async Task<IActionResult> GetStatus(Guid id)
    {
        var doc = await _db.Documents.FindAsync(id);
        if (doc == null) return NotFound();

        return Ok(new { doc.Id, doc.Filename, Status = doc.Status.ToString(), doc.Confidence, doc.DocumentType });
    }

    /// <summary>
    /// Get extracted fields for a document.
    /// </summary>
    [HttpGet("{id:guid}/extracted")]
    public async Task<IActionResult> GetExtracted(Guid id)
    {
        var doc = await _db.Documents
            .Include(d => d.ExtractedFields)
            .FirstOrDefaultAsync(d => d.Id == id);

        if (doc == null) return NotFound();

        // Derive the stored filename from the raw file path for image display
        var storedFilename = doc.RawFilePath != null
            ? Path.GetFileName(doc.RawFilePath)
            : doc.Filename;

        return Ok(new
        {
            doc.Id,
            doc.Filename,
            storedFilename,
            doc.DocumentType,
            doc.Confidence,
            doc.RawText,
            Status = doc.Status.ToString(),
            Fields = doc.ExtractedFields.Select(f => new
            {
                f.Id,
                f.FieldName,
                f.ExtractedValue,
                f.CorrectedValue,
                f.Confidence,
                f.BoundingBoxJson,
                f.IsHumanCorrected
            })
        });
    }

    /// <summary>
    /// Submit human validation corrections for a document.
    /// </summary>
    [HttpPost("{id:guid}/validate")]
    public async Task<IActionResult> Validate(Guid id, [FromBody] ValidateRequest request)
    {
        var doc = await _db.Documents
            .Include(d => d.ExtractedFields)
            .FirstOrDefaultAsync(d => d.Id == id);

        if (doc == null) return NotFound();

        if (request.Corrections != null)
        {
            foreach (var correction in request.Corrections)
            {
                var field = doc.ExtractedFields.FirstOrDefault(f => f.FieldName == correction.FieldName);
                if (field != null)
                {
                    _db.Corrections.Add(new Correction
                    {
                        DocumentId = id,
                        FieldName = field.FieldName,
                        OriginalValue = field.ExtractedValue,
                        CorrectedValue = correction.CorrectedValue
                    });

                    field.CorrectedValue = correction.CorrectedValue;
                    field.IsHumanCorrected = true;
                }
            }
        }

        doc.Status = request.Status switch
        {
            "rejected" => DocumentStatus.Failed,
            _ => DocumentStatus.Validated
        };

        await _db.SaveChangesAsync();
        return Ok(new { doc.Id, doc.Status });
    }

    /// <summary>
    /// Trigger export for a validated document.
    /// Delivers to the configured export target (webhook, postgres, or file).
    /// </summary>
    [HttpPost("{id:guid}/export")]
    public async Task<IActionResult> Export(Guid id, [FromServices] ExportService exportService)
    {
        var result = await exportService.ExportAsync(id);
        if (!result.Success)
            return BadRequest(new { error = result.Error });

        return Ok(new { id, status = "exported", destination = result.Destination });
    }
}

public class ValidateRequest
{
    public string? Status { get; set; }
    public List<FieldCorrection>? Corrections { get; set; }
}

public class FieldCorrection
{
    public string FieldName { get; set; } = string.Empty;
    public string CorrectedValue { get; set; } = string.Empty;
}