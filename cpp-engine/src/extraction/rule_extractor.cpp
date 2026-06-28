#include "rule_extractor.hpp"

#include <format>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace clearcapture {

// Built-in Rule Sets

std::unordered_map<std::string, RuleSet> RuleExtractor::builtin_rules() {
    std::unordered_map<std::string, RuleSet> rules;

    // Invoice extraction rules
    rules["invoice"] = RuleSet{
        "invoice",
        {
            {"vendor_name",     R"(Vendor[:\s]+(.+))",                                       "header"},
            {"invoice_number",  R"(Invoice\s*(?:#|No|Number)[:\s]*([A-Za-z0-9\-/]+))",       "header"},
            {"date",            R"(Date[:\s]*(\d{1,2}[/-]\d{1,2}[/-]\d{2,4}))",              "header"},
            {"date_alt",        R"((\d{4}-\d{2}-\d{2}))",                                     "header"},
            {"total_amount",    R"(Total[:\s]*\$?([\d,]+\.\d{2}))",                           "footer"},
            {"subtotal",        R"(Subtotal[:\s]*\$?([\d,]+\.\d{2}))",                        "body"},
            {"tax_amount",      R"(Tax[:\s]*\$?([\d,]+\.\d{2}))",                             "body"},
            {"line_items",      R"((?:Item|Description)(?:.|\n)*?(?=\nTotal|$))",              "body"},
            {"due_date",        R"(Due[:\s]*(\d{1,2}[/-]\d{1,2}[/-]\d{2,4}))",               "footer"},
            {"po_number",       R"(PO\s*(?:#|Number)?[:\s]*([A-Za-z0-9\-/]+))",               "header"},
        }
    };

    // Receipt extraction rules
    rules["receipt"] = RuleSet{
        "receipt",
        {
            {"merchant_name",   R"(^(.+)\n)",                                                "header"},
            {"date",            R"((\d{1,2}[/-]\d{1,2}[/-]\d{2,4}))",                        "header"},
            {"total_amount",    R"(Total[:\s]*\$?([\d,]+\.\d{2}))",                           "footer"},
            {"subtotal",        R"(Subtotal[:\s]*\$?([\d,]+\.\d{2}))",                        "footer"},
            {"tax_amount",      R"(Tax[:\s]*\$?([\d,]+\.\d{2}))",                             "footer"},
            {"payment_method",  R"(VISA|MASTERCARD|AMEX|CASH|DEBIT)",                         "footer"},
            {"card_last_four",  R"((?:\*{2,}|X{2,})(\d{4}))",                                 "footer"},
        }
    };

    // Medical form extraction rules
    rules["medical_form"] = RuleSet{
        "medical_form",
        {
            {"patient_name",    R"(PATIENT NAME[:\s]+(.+))",                                  "header"},
            {"date_of_birth",   R"(DATE OF BIRTH[:\s]*(\d{4}-\d{2}-\d{2}))",                  "header"},
            {"dob_alt",         R"((\d{4}-\d{2}-\d{2}))",                                     "header"},
            {"diagnosis_code",  R"((?:ICD|DX)[:\s]*([A-Z]\d{2}(?:\.\d{1,2})?))",              "body"},
            {"provider_name",   R"(PHYSICIAN[:\s]+(.+))",                                     "header"},
            {"insurance",       R"(INSURANCE[:\s]+(.+))",                                     "header"},
            {"policy_number",   R"(POLICY[:\s]+(.+))",                                        "header"},
        }
    };

    // Legal contract extraction rules
    rules["legal_contract"] = RuleSet{
        "legal_contract",
        {
            {"agreement_date",  R"(Date[:\s]*(\d{1,2}[/-]\d{1,2}[/-]\d{2,4}))",               "header"},
            {"party_name",      R"(AGREEMENT[.\s]+between[:\s]+(.+?)(?:and|hereby))",          "header"},
            {"effective_date",  R"(Effective[:\s]*(\d{1,2}[/-]\d{1,2}[/-]\d{2,4}))",          "header"},
            {"contract_number", R"(Contract\s*(?:#|No)?[:\s]*([A-Za-z0-9\-/]+))",              "header"},
            {"jurisdiction",    R"(State of\s+([A-Za-z\s]+))",                                 "footer"},
        }
    };

    // Government ID extraction rules
    rules["government_id"] = RuleSet{
        "government_id",
        {
            {"full_name",       R"((?:Name|Full Name)[:\s]+(.+))",                             "header"},
            {"date_of_birth",   R"(DOB[:\s]*(\d{1,2}[/-]\d{1,2}[/-]\d{2,4}))",                "body"},
            {"id_number",       R"(ID[:\s]*(?:#|No)?[:\s]*([A-Z0-9\-]+))",                    "header"},
            {"expiry_date",     R"(EXP[:\s]*(\d{1,2}[/-]\d{1,2}[/-]\d{2,4}))",                 "footer"},
            {"nationality",     R"(Nationality[:\s]+(.+))",                                    "body"},
        }
    };

    return rules;
}

// Constructor

RuleExtractor::RuleExtractor() {
    rules_ = builtin_rules();
}

// Register custom rules

void RuleExtractor::register_rules(const RuleSet& rules) {
    rules_[rules.document_type] = rules;
}

// Extract fields from OCR text

std::vector<ExtractedField> RuleExtractor::extract(
    const std::string& ocr_text,
    const std::string& document_type)
{
    auto it = rules_.find(document_type);
    if (it == rules_.end()) {
        return {};
    }

    std::vector<ExtractedField> results;

    for (const auto& rule : it->second.rules) {
        std::smatch match;
        if (std::regex_search(ocr_text, match, rule.pattern)) {
            ExtractedField field;
            field.field_name = rule.field_name;
            field.value = match.size() > 1 ? match[1].str() : match[0].str();
            // Trim whitespace
            field.value.erase(0, field.value.find_first_not_of(" \t\r\n"));
            field.value.erase(field.value.find_last_not_of(" \t\r\n") + 1);
            field.confidence = 85.0; // Rule-based extraction is generally reliable
            field.document_type = document_type;
            results.push_back(field);
        }
    }

    std::cout << std::format("[Extract] Extracted {} fields from {} using rules\n",
                             results.size(), document_type);

    return results;
}

// Find bounding box for a value in symbol boxes

std::optional<BoundingBox> RuleExtractor::find_bounding_box(
    const std::string& value,
    const std::vector<OCRResult::SymbolBox>& symbols) const
{
    // Search for a symbol that contains the value (or vice versa)
    for (const auto& sym : symbols) {
        // Check if the symbol text matches the value
        std::string sym_lower = sym.symbol;
        std::string val_lower = value;
        std::transform(sym_lower.begin(), sym_lower.end(), sym_lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        std::transform(val_lower.begin(), val_lower.end(), val_lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (sym_lower.find(val_lower) != std::string::npos ||
            val_lower.find(sym_lower) != std::string::npos) {
            return BoundingBox{sym.x, sym.y, sym.width, sym.height};
        }
    }

    return std::nullopt;
}

// Extract with bounding boxes

std::vector<ExtractedField> RuleExtractor::extract_with_boxes(
    const std::string& ocr_text,
    const std::vector<OCRResult::SymbolBox>& symbols,
    const std::string& document_type)
{
    auto fields = extract(ocr_text, document_type);

    // Try to find bounding boxes for each field
    for (auto& field : fields) {
        auto bb = find_bounding_box(field.value, symbols);
        if (bb.has_value()) {
            field.bounding_box = bb;
        }
    }

    return fields;
}

} // namespace clearcapture