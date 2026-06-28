namespace DocForge.Api.Models;

/// <summary>
/// Mock ERP/CRM table for the postgres_write export target.
/// Demonstrates that validated document data can be written to
/// a normalized downstream system schema.
/// </summary>
public class MockErpOrder
{
    public Guid Id { get; set; } = Guid.NewGuid();
    public Guid DocumentId { get; set; }
    public string DocumentType { get; set; } = string.Empty;
    public string Filename { get; set; } = string.Empty;
    public string PayloadJson { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}