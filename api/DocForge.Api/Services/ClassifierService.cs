using System.Net.Http.Json;
using System.Text.Json.Serialization;

namespace DocForge.Api.Services;

/// <summary>
/// HTTP client wrapper for the Python ML classifier microservice.
/// Called by the DocumentProcessingService after OCR extraction
/// to classify document type (the "Classify" step in the pipeline).
/// </summary>
public class ClassifierService
{
    private readonly HttpClient _http;
    private readonly ILogger<ClassifierService> _logger;

    public ClassifierService(HttpClient http, ILogger<ClassifierService> logger)
    {
        _http = http;
        _logger = logger;
    }

    /// <summary>
    /// Classify a document's OCR text into a document type.
    /// </summary>
    /// <param name="rawText">The raw OCR text from the C++ engine.</param>
    /// <returns>Classification result, or null if the classifier is unavailable.</returns>
    public async Task<ClassifyResult?> ClassifyAsync(string rawText)
    {
        if (string.IsNullOrWhiteSpace(rawText))
        {
            return null;
        }

        try
        {
            var response = await _http.PostAsJsonAsync("/classify", new { text = rawText });
            response.EnsureSuccessStatusCode();
            var result = await response.Content.ReadFromJsonAsync<ClassifyResult>();
            return result;
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Classifier service unavailable, falling back to default type");
            return null;
        }
    }
}

public class ClassifyResult
{
    [JsonPropertyName("document_type")]
    public string DocumentType { get; set; } = "unknown";

    [JsonPropertyName("confidence")]
    public double Confidence { get; set; }

    [JsonPropertyName("needs_review")]
    public bool NeedsReview { get; set; }
}