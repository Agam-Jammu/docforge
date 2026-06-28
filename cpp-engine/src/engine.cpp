#include "engine.hpp"
#include "thread_pool/thread_pool.hpp"
#include "preprocessing/image_processor.hpp"
#include "ocr/tesseract_engine.hpp"
#include "extraction/rule_extractor.hpp"

#include <format>
#include <iostream>
#include <memory>

namespace docforge {

// Global engine state
static std::unique_ptr<TesseractEngine> g_ocr_engine;
static std::unique_ptr<RuleExtractor> g_extractor;
static std::unique_ptr<ImageProcessor> g_image_processor;

bool initialize_engine(const std::string& tessdata_path) {
    try {
        // Create all components before assigning to globals (no partial init)
        auto ocr = std::make_unique<TesseractEngine>("eng", tessdata_path);
        auto extractor = std::make_unique<RuleExtractor>();
        auto processor = std::make_unique<ImageProcessor>();

        // All created successfully — now assign to globals
        g_ocr_engine = std::move(ocr);
        g_extractor = std::move(extractor);
        g_image_processor = std::move(processor);

        std::cout << std::format("[Engine] DocForge C++ engine initialized\n"
                                 "  Threads: {}\n"
                                 "  Tesseract: ready\n",
                                 std::thread::hardware_concurrency());
        return true;
    } catch (const std::exception& e) {
        std::cerr << std::format("[Engine] Initialization failed: {}\n", e.what());
        return false;
    }
}

Result<DocumentResult> process_document(const std::string& filepath, const std::string& document_type) {
    if (!g_ocr_engine || !g_extractor || !g_image_processor) {
        return Result<DocumentResult>("engine not initialized — call initialize_engine() first");
    }

    fs::path path(filepath);

    // Step 1: Capture — memory-map the file
    std::cout << std::format("[Capture] Loading: {}\n", path.string());

    MemoryMappedFile mmf(path);
    auto buffer = mmf.get_span();

    // Step 2: Preprocess
    auto preprocess_result = g_image_processor->preprocess(buffer);
    if (!preprocess_result) {
        return Result<DocumentResult>(std::format("preprocessing failed: {}", preprocess_result.error));
    }

    cv::Mat processed = *preprocess_result;

    // Step 3: OCR
    auto ocr_result = g_ocr_engine->recognize(processed.data, processed.cols, processed.rows, 1);
    if (!ocr_result) {
        return Result<DocumentResult>(std::format("OCR failed: {}", ocr_result.error));
    }

    // Step 4: Rule-based extraction (Strategy A)
    // If document_type is "unknown", auto-detect by trying all rule sets
    // and picking the one with the most extracted fields.
    std::string detected_type = document_type;
    std::vector<ExtractedField> fields;

    if (document_type == "unknown") {
        const auto& builtin = RuleExtractor::builtin_rules();
        size_t best_count = 0;
        std::string best_type = "unknown";

        for (const auto& [type, _] : builtin) {
            auto candidate = g_extractor->extract_with_boxes(
                ocr_result->text, ocr_result->symbols, type);
            if (candidate.size() > best_count) {
                best_count = candidate.size();
                best_type = type;
                fields = std::move(candidate);
            }
        }

        std::cout << std::format("[Classify] Auto-detected type: {} ({} fields)\n",
                                 best_type, best_count);
        detected_type = best_type;
    } else {
        fields = g_extractor->extract_with_boxes(
            ocr_result->text, ocr_result->symbols, document_type);
    }

    // Build document result
    DocumentResult result;
    result.filename = path.filename().string();
    result.document_type = detected_type;
    result.confidence = g_ocr_engine->mean_confidence();
    result.fields = std::move(fields);
    result.raw_text = std::move(ocr_result->text);
    result.page_count = 1;

    std::cout << std::format("[Pipeline] Completed: {} (conf: {:.1f}%, fields: {})\n",
                             result.filename, result.confidence, result.fields.size());

    return result;
}

// Shutdown function called by the extern "C" wrappers
void shutdown_engine() {
    g_ocr_engine.reset();
    g_extractor.reset();
    g_image_processor.reset();
    std::cout << "[Engine] Shutdown complete\n";
}

} // namespace docforge