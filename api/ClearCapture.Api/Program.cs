using ClearCapture.Api.Data;
using ClearCapture.Api.Services;
using Microsoft.EntityFrameworkCore;

var builder = WebApplication.CreateBuilder(args);

// Database
builder.Services.AddDbContext<AppDbContext>(options =>
    options.UseNpgsql(builder.Configuration.GetConnectionString("DefaultConnection")));

// C++ Engine (P/Invoke) — singleton because it manages native resources
builder.Services.AddSingleton<CppEngineService>();

// ML Classifier client — calls the Python FastAPI service
builder.Services.AddHttpClient<ClassifierService>(client =>
{
    client.BaseAddress = new Uri(builder.Configuration["ClassifierUrl"] ?? "http://localhost:8000");
    client.Timeout = TimeSpan.FromSeconds(10);
});

// Export service — handles delivery to configured targets
builder.Services.AddSingleton<ExportService>();

// Background document processing service
builder.Services.AddHostedService<DocumentProcessingService>();

// Controllers
builder.Services.AddControllers();
builder.Services.AddOpenApi();

// CORS for React frontend
builder.Services.AddCors(options =>
{
    options.AddDefaultPolicy(policy =>
    {
        policy.AllowAnyOrigin().AllowAnyMethod().AllowAnyHeader();
    });
});

var app = builder.Build();

// Auto-migrate database on startup
using (var scope = app.Services.CreateScope())
{
    var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
    db.Database.EnsureCreated();
}

// Serve uploaded files for the React UI to display document images
var uploadsDir = Path.Combine(builder.Environment.ContentRootPath, "Uploads");
Directory.CreateDirectory(uploadsDir);
app.UseStaticFiles(new StaticFileOptions
{
    FileProvider = new Microsoft.Extensions.FileProviders.PhysicalFileProvider(uploadsDir),
    RequestPath = "/uploads",
    OnPrepareResponse = ctx =>
    {
        ctx.Context.Response.Headers.Append("Access-Control-Allow-Origin", "*");
    }
});

if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.UseCors();
app.MapControllers();

app.Run();