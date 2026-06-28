#include "tesseract_engine.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <format>
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace docforge {

TesseractEngine::TesseractEngine(const std::string& language,
                                 const std::string& tessdata_path) {
    api_ = std::make_unique<tesseract::TessBaseAPI>();

    int rc;
    if (tessdata_path.empty()) {
        rc = api_->Init(nullptr, language.c_str());
    } else {
        rc = api_->Init(tessdata_path.c_str(), language.c_str());
    }

    if (rc != 0) {
        throw std::runtime_error(
            std::format("failed to initialize Tesseract with language '{}'", language));
    }

    api_->SetVariable("tessedit_char_whitelist", "");
    api_->SetVariable("classify_bln_numeric_mode", "0");
    api_->SetPageSegMode(tesseract::PSM_AUTO);

    std::cout << std::format("[OCR] Tesseract engine initialized (language: {})\n", language);
}

TesseractEngine::~TesseractEngine() {
    if (api_) {
        api_->End();
    }
}

TesseractEngine::TesseractEngine(TesseractEngine&& other) noexcept
    : api_(std::move(other.api_))
    , mean_conf_(other.mean_conf_)
{
    other.mean_conf_ = 0.0;
}

TesseractEngine& TesseractEngine::operator=(TesseractEngine&& other) noexcept {
    if (this != &other) {
        if (api_) api_->End();
        api_ = std::move(other.api_);
        mean_conf_ = other.mean_conf_;
        other.mean_conf_ = 0.0;
    }
    return *this;
}

// OCR Recognition

Result<OCRResult> TesseractEngine::recognize(
    const unsigned char* image_data,
    int width,
    int height,
    int bytes_per_pixel)
{
    if (!api_) {
        return Result<OCRResult>("Tesseract engine not initialized");
    }

    api_->SetImage(image_data, width, height, bytes_per_pixel, width * bytes_per_pixel);

    char* text = api_->GetUTF8Text();
    if (!text) {
        return Result<OCRResult>("OCR returned no text");
    }

    OCRResult result;
    result.text = std::string(text);
    result.page_number = 1;
    result.width = width;
    result.height = height;
    delete[] text;

    result.symbols.clear();

    tesseract::ResultIterator* ri = api_->GetIterator();
    tesseract::PageIteratorLevel level = tesseract::RIL_WORD;

    if (ri) {
        double total_conf = 0.0;
        int conf_count = 0;

        do {
            const char* word = ri->GetUTF8Text(level);
            if (word == nullptr) continue;

            float conf = ri->Confidence(level);
            int x1, y1, x2, y2;
            ri->BoundingBox(level, &x1, &y1, &x2, &y2);

            OCRResult::SymbolBox box;
            box.symbol = std::string(word);
            box.x = x1;
            box.y = y1;
            box.width = x2 - x1;
            box.height = y2 - y1;
            box.confidence = static_cast<double>(conf);

            result.symbols.push_back(box);
            total_conf += conf;
            conf_count++;

            delete[] word;
        } while (ri->Next(level));

        mean_conf_ = (conf_count > 0) ? (total_conf / conf_count) : 0.0;
        delete ri;
    }

    std::cout << std::format("[OCR] Recognized {} words (mean confidence: {:.1f}%)\n",
                             result.symbols.size(), mean_conf_);

    return result;
}

Result<OCRResult> TesseractEngine::recognize_from_buffer(
    std::span<const char> buffer)
{
    std::vector<unsigned char> vec(buffer.begin(), buffer.end());
    cv::Mat img = cv::imdecode(vec, cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        return Result<OCRResult>("failed to decode image buffer for OCR");
    }

    return recognize(img.data, img.cols, img.rows, 1);
}

} // namespace docforge