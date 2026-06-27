using ClearCapture.Api.Data;
using ClearCapture.Api.Models;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace ClearCapture.Api.Controllers;

[ApiController]
[Route("api/admin")]
public class AdminController : ControllerBase
{
    private readonly AppDbContext _db;

    public AdminController(AppDbContext db)
    {
        _db = db;
    }

    /// <summary>
    /// Get all workflow configurations.
    /// </summary>
    [HttpGet("workflows")]
    public async Task<IActionResult> GetWorkflows()
    {
        var workflows = await _db.WorkflowConfigs.ToListAsync();
        return Ok(workflows);
    }

    /// <summary>
    /// Update workflow configuration for a document type.
    /// </summary>
    [HttpPut("workflows")]
    public async Task<IActionResult> UpsertWorkflow([FromBody] WorkflowConfig config)
    {
        var existing = await _db.WorkflowConfigs
            .FirstOrDefaultAsync(w => w.DocumentType == config.DocumentType);

        if (existing != null)
        {
            existing.ExportTarget = config.ExportTarget;
            existing.ExportConfigJson = config.ExportConfigJson;
        }
        else
        {
            _db.WorkflowConfigs.Add(new WorkflowConfig
            {
                DocumentType = config.DocumentType,
                ExportTarget = config.ExportTarget,
                ExportConfigJson = config.ExportConfigJson
            });
        }

        await _db.SaveChangesAsync();
        return Ok(new { status = "updated", documentType = config.DocumentType });
    }
}