#pragma once

#include <string>
#include <vector>
#include <format>
#include <optional>

namespace clearcapture {

/**
 * @brief Represents the bounding box of a text region in the document image.
 */
struct BoundingBox {
    int x;
    int y;
    int width;
    int height;

    [[nodiscard]] std::string to_json() const {
        return std::format(R"({{"x":{},"y":{},"width":{},"height":{}}})", x, y, width, height);
    }
};

/**
 * @brief Represents a single extracted field from a document.
 *
 * Stores the field name, extracted value, confidence score,
 * and bounding box coordinates for UI overlay rendering.
 */
struct ExtractedField {
    std::string field_name;
    std::string value;
    double confidence;
    std::optional<BoundingBox> bounding_box;
    std::string document_type;

    [[nodiscard]] std::string to_json() const {
        std::string bb_json = bounding_box.has_value()
            ? bounding_box->to_json()
            : "null";
        return std::format(
            R"({{"field_name":"{}","value":"{}","confidence":{:.2f},"bounding_box":{},"document_type":"{}"}})",
            field_name, value, confidence, bb_json, document_type
        );
    }
};

/**
 * @brief The result of processing a single document.
 */
struct DocumentResult {
    std::string filename;
    std::string document_type;
    double confidence;
    std::vector<ExtractedField> fields;
    std::string raw_text;
    int page_count;

    [[nodiscard]] std::string to_json() const {
        std::string fields_json;
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) fields_json += ",";
            fields_json += fields[i].to_json();
        }
        return std::format(
            R"({{"filename":"{}","document_type":"{}","confidence":{:.2f},"page_count":{},"fields":[{}],"raw_text":"{}"}})",
            filename, document_type, confidence, page_count, fields_json, raw_text
        );
    }
};

} // namespace clearcapture