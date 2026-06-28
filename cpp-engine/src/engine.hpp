#pragma once

#include "extraction/extracted_field.hpp"
#include "ingestion/file_loader.hpp"

namespace docforge {

/**
 * @brief Initialize the capture engine.
 * Must be called once before any other engine functions.
 */
bool initialize_engine(const std::string& tessdata_path = "");

/**
 * @brief Process a single document through the full pipeline.
 */
Result<DocumentResult> process_document(const std::string& filepath, const std::string& document_type = "invoice");

/**
 * @brief Shutdown the engine and release all resources.
 */
void shutdown_engine();

} // namespace docforge
