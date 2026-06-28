/**
 * @file main.cpp
 * @brief DocForge C++20 Engine — Shared Library Entry Point.
 *
 * This is the primary interface for .NET Core P/Invoke integration.
 * Exports C-compatible functions that the .NET orchestrator calls.
 */

#include "engine.hpp"

#include <format>
#include <iostream>
#include <cstring>
#include <cstdlib>

// C-compatible exports for .NET P/Invoke

extern "C" {

/**
 * @brief Initialize the engine. Call once before any other function.
 * @return 0 on success, -1 on failure.
 */
int DOCFORGE_Initialize(const char* tessdata_path) {
    std::string path = tessdata_path ? std::string(tessdata_path) : "";
    return docforge::initialize_engine(path) ? 0 : -1;
}

/**
 * @brief Process a document and return JSON result.
 * Caller must DOCFORGE_FreeString the returned string.
 *
 * @param filepath Path to the document file.
 * @return JSON string with document result, or NULL on error.
 */
char* DOCFORGE_ProcessDocument(const char* filepath) {
    auto result = docforge::process_document(filepath);
    if (!result.ok) {
        return nullptr;
    }

    std::string json = result.value.to_json();
    char* cstr = static_cast<char*>(std::malloc(json.size() + 1));
    if (cstr) {
        std::memcpy(cstr, json.c_str(), json.size() + 1);
    }
    return cstr;
}

/**
 * @brief Free a string allocated by the engine.
 */
void DOCFORGE_FreeString(char* str) {
    std::free(str);
}

/**
 * @brief Shutdown the engine and release all resources.
 */
void DOCFORGE_Shutdown() {
    docforge::shutdown_engine();
}

} // extern "C"