using Microsoft.EntityFrameworkCore;
using ClearCapture.Api.Models;

namespace ClearCapture.Api.Data;

public class AppDbContext : DbContext
{
    public AppDbContext(DbContextOptions<AppDbContext> options) : base(options) { }

    public DbSet<Document> Documents => Set<Document>();
    public DbSet<ExtractedField> ExtractedFields => Set<ExtractedField>();
    public DbSet<Correction> Corrections => Set<Correction>();
    public DbSet<WorkflowConfig> WorkflowConfigs => Set<WorkflowConfig>();
    public DbSet<ExportLog> ExportLogs => Set<ExportLog>();

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        modelBuilder.Entity<Document>(entity =>
        {
            entity.HasKey(e => e.Id);
            entity.Property(e => e.Filename).HasMaxLength(500).IsRequired();
            entity.Property(e => e.DocumentType).HasMaxLength(100);
            entity.Property(e => e.Status)
                  .HasConversion<string>()
                  .HasMaxLength(20);
            entity.HasIndex(e => e.Status).HasFilter("\"Status\" = 'Pending'");
        });

        modelBuilder.Entity<ExtractedField>(entity =>
        {
            entity.HasKey(e => e.Id);
            entity.Property(e => e.FieldName).HasMaxLength(200).IsRequired();
            entity.HasOne(e => e.Document)
                  .WithMany(d => d.ExtractedFields)
                  .HasForeignKey(e => e.DocumentId)
                  .OnDelete(DeleteBehavior.Cascade);
            entity.HasIndex(e => e.DocumentId);
        });

        modelBuilder.Entity<Correction>(entity =>
        {
            entity.HasKey(e => e.Id);
            entity.Property(e => e.FieldName).HasMaxLength(200).IsRequired();
            entity.HasIndex(e => e.DocumentId);
        });

        modelBuilder.Entity<WorkflowConfig>(entity =>
        {
            entity.HasKey(e => e.Id);
            entity.Property(e => e.DocumentType).HasMaxLength(100).IsRequired();
            entity.HasIndex(e => e.DocumentType).IsUnique();
        });

        modelBuilder.Entity<ExportLog>(entity =>
        {
            entity.HasKey(e => e.Id);
            entity.HasIndex(e => e.DocumentId);
        });
    }
}