namespace DocForge.Api.Models;

public enum DocumentStatus
{
    Pending,
    Processing,
    Validated,
    Exported,
    Failed
}

public class Document
{
    public Guid Id { get; set; } = Guid.NewGuid();
    public string Filename { get; set; } = string.Empty;
    public string DocumentType { get; set; } = "unknown";
    public DocumentStatus Status { get; set; } = DocumentStatus.Pending;
    public double Confidence { get; set; }
    public DateTime UploadedAt { get; set; } = DateTime.UtcNow;
    public string? RawFilePath { get; set; }
    public string? RawText { get; set; }

    public ICollection<ExtractedField> ExtractedFields { get; set; } = new List<ExtractedField>();
}

public class ExtractedField
{
    public Guid Id { get; set; } = Guid.NewGuid();
    public Guid DocumentId { get; set; }
    public string FieldName { get; set; } = string.Empty;
    public string ExtractedValue { get; set; } = string.Empty;
    public string? CorrectedValue { get; set; }
    public string? BoundingBoxJson { get; set; }
    public double Confidence { get; set; }
    public bool IsHumanCorrected { get; set; }

    public Document Document { get; set; } = null!;
}

public class Correction
{
    public Guid Id { get; set; } = Guid.NewGuid();
    public Guid DocumentId { get; set; }
    public string FieldName { get; set; } = string.Empty;
    public string OriginalValue { get; set; } = string.Empty;
    public string CorrectedValue { get; set; } = string.Empty;
    public DateTime CorrectedAt { get; set; } = DateTime.UtcNow;
}

public class WorkflowConfig
{
    public Guid Id { get; set; } = Guid.NewGuid();
    public string DocumentType { get; set; } = string.Empty;
    public string ExportTarget { get; set; } = "json_webhook";
    public string ExportConfigJson { get; set; } = "{}";
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

public class ExportLog
{
    public Guid Id { get; set; } = Guid.NewGuid();
    public Guid DocumentId { get; set; }
    public DateTime ExportedAt { get; set; } = DateTime.UtcNow;
    public string Destination { get; set; } = string.Empty;
    public string Status { get; set; } = "pending";
    public string? ResponseJson { get; set; }
}