#pragma once

#include <string>
#include <vector>
#include <regex>
#include <functional>
#include <unordered_map>

#include "extraction/extracted_field.hpp"
#include "ocr/tesseract_engine.hpp"

namespace clearcapture {

/**
 * @brief Extraction rule: a named field with a regex pattern and positional hint.
 *
 * Rules are tried in order. The regex extracts the value from OCR text.
 * The positional_hint helps locate the field on the document for bounding box
 * estimation when exact coordinates aren't available from Tesseract's iterator.
 */
struct ExtractionRule {
    std::string field_name;
    std::regex pattern;
    std::string positional_hint; // e.g., "top-right", "header", "footer"

    ExtractionRule(const std::string& name, const std::string& regex_pattern,
                   const std::string& hint = "")
        : field_name(name)
        , pattern(regex_pattern, std::regex::icase | std::regex::optimize)
        , positional_hint(hint)
    {}
};

/**
 * @brief Collection of extraction rules for a specific document type.
 */
struct RuleSet {
    std::string document_type;
    std::vector<ExtractionRule> rules;
};

/**
 * @brief Rule-based field extraction engine (Strategy A).
 *
 * Applies regex patterns to OCR text to extract structured fields.
 * Fast and deterministic — ideal for well-structured documents
 * like invoices with consistent layouts.
 *
 * This is the "cascading recognition" Strategy A. If this fails
 * (low confidence), the .NET orchestrator falls back to the ML-based
 * NER extraction (Strategy B, a Python service).
 */
class RuleExtractor {
public:
    RuleExtractor();

    /**
     * @brief Extract fields from OCR text given a document type.
     *
     * @param ocr_text The raw text output from Tesseract.
     * @param document_type The classified document type (e.g., "invoice", "receipt").
     * @return std::vector<ExtractedField> with values and confidence scores.
     */
    [[nodiscard]] std::vector<ExtractedField> extract(
        const std::string& ocr_text,
        const std::string& document_type);

    /**
     * @brief Register a custom rule set for a document type.
     */
    void register_rules(const RuleSet& rules);

    /**
     * @brief Get the built-in rule sets.
     */
    static std::unordered_map<std::string, RuleSet> builtin_rules();

    /**
     * @brief Extract using the OCR result's symbol bounding boxes.
     * This enables "data type highlighting" — mapping extracted fields
     * back to their positions on the document image.
     */
    [[nodiscard]] std::vector<ExtractedField> extract_with_boxes(
        const std::string& ocr_text,
        const std::vector<OCRResult::SymbolBox>& symbols,
        const std::string& document_type);

private:
    /**
     * @brief Find the bounding box for an extracted value by searching
     * through the symbol boxes.
     */
    [[nodiscard]] std::optional<BoundingBox> find_bounding_box(
        const std::string& value,
        const std::vector<OCRResult::SymbolBox>& symbols) const;

    std::unordered_map<std::string, RuleSet> rules_;
};

} // namespace clearcapture