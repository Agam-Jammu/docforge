#pragma once

#include <span>
#include <string>
#include <vector>
#include <memory>

#include <tesseract/baseapi.h>

#include "extraction/extracted_field.hpp"
#include "ingestion/file_loader.hpp"

namespace clearcapture {

/**
 * @brief Result of OCR processing on a single page.
 */
struct OCRResult {
    std::string text;
    int page_number;
    int width;
    int height;

    /**
     * @brief Get bounding boxes for each recognized symbol/word.
     * These are used by the UI to highlight field locations ("data type highlighting").
     */
    struct SymbolBox {
        std::string symbol;
        int x, y, width, height;
        double confidence;

        [[nodiscard]] BoundingBox to_bounding_box() const {
            return {x, y, width, height};
        }
    };

    std::vector<SymbolBox> symbols;
};

/**
 * @brief Tesseract 5.x OCR engine wrapper.
 *
 * Takes preprocessed image data (grayscale pixels) and produces
 * recognized text with per-symbol bounding boxes and confidence scores.
 */
class TesseractEngine {
public:
    explicit TesseractEngine(const std::string& language = "eng",
                              const std::string& tessdata_path = "");

    ~TesseractEngine();

    TesseractEngine(const TesseractEngine&) = delete;
    TesseractEngine& operator=(const TesseractEngine&) = delete;
    TesseractEngine(TesseractEngine&& other) noexcept;
    TesseractEngine& operator=(TesseractEngine&& other) noexcept;

    [[nodiscard]] Result<OCRResult> recognize(
        const unsigned char* image_data,
        int width,
        int height,
        int bytes_per_pixel = 1);

    [[nodiscard]] Result<OCRResult> recognize_from_buffer(
        std::span<const char> buffer);

    [[nodiscard]] double mean_confidence() const { return mean_conf_; }
    [[nodiscard]] bool is_ready() const { return api_ != nullptr; }

private:
    std::unique_ptr<tesseract::TessBaseAPI> api_;
    double mean_conf_ = 0.0;
};

} // namespace clearcapture