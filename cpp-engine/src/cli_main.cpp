/**
 * @file cli_main.cpp
 * @brief CLI test runner for the ClearCapture C++ engine.
 *
 * Usage: clearcapture_cli <file1> [file2] ...
 *
 * Processes documents through the full pipeline:
 *   Capture (mmap) → Preprocess (OpenCV) → OCR (Tesseract) → Extract (rules)
 * Outputs JSON results to stdout.
 */

#include "engine.hpp"

#include <iostream>
#include <format>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << std::format("Usage: {} [--type <doc_type>] <document1> [document2 ...]\n", argv[0]);
        return 1;
    }

    std::string document_type = "invoice";
    int file_start = 1;

    // Parse optional --type argument
    if (argc >= 4 && std::string(argv[1]) == "--type") {
        document_type = argv[2];
        file_start = 3;
    }

    if (!clearcapture::initialize_engine()) {
        std::cerr << "Failed to initialize engine\n";
        return 1;
    }

    std::vector<std::string> files;
    for (int i = file_start; i < argc; ++i) {
        files.push_back(argv[i]);
    }

    std::cout << "[\n";
    for (size_t i = 0; i < files.size(); ++i) {
        auto result = clearcapture::process_document(files[i], document_type);
        if (result.ok) {
            std::cout << "  " << result.value.to_json();
        } else {
            std::cout << std::format(R"({{"filename":"{}","error":"{}"}})",
                                     files[i], result.error);
        }
        if (i < files.size() - 1) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "]\n";

    clearcapture::shutdown_engine();

    return 0;
}