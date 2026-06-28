cmake_minimum_required(VERSION 3.28)
project(docforge_engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find dependencies
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TESSERACT REQUIRED IMPORTED_TARGET tesseract)
pkg_check_modules(POPPLER REQUIRED IMPORTED_TARGET poppler)

# Source files
set(SOURCES
    src/main.cpp
    src/engine.cpp
    src/thread_pool/thread_pool.cpp
    src/ingestion/file_loader.cpp
    src/preprocessing/image_processor.cpp
    src/ocr/tesseract_engine.cpp
    src/extraction/rule_extractor.cpp
)

set(HEADERS
    src/engine.hpp
    src/thread_pool/thread_pool.hpp
    src/ingestion/file_loader.hpp
    src/preprocessing/image_processor.hpp
    src/ocr/tesseract_engine.hpp
    src/extraction/rule_extractor.hpp
    src/extraction/extracted_field.hpp
)

# Build shared library
add_library(docforge_engine SHARED ${SOURCES} ${HEADERS})

target_include_directories(docforge_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${TESSERACT_INCLUDE_DIRS}
        ${POPPLER_INCLUDE_DIRS}
)

target_link_libraries(docforge_engine
    PUBLIC
        ${OpenCV_LIBS}
        PkgConfig::TESSERACT
        PkgConfig::POPPLER
)

target_compile_features(docforge_engine PUBLIC cxx_std_20)

# Also build a CLI test runner
add_executable(docforge_cli
    src/cli_main.cpp
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(docforge_cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OpenCV_INCLUDE_DIRS}
        ${TESSERACT_INCLUDE_DIRS}
        ${POPPLER_INCLUDE_DIRS}
)

target_link_libraries(docforge_cli
    PRIVATE
        ${OpenCV_LIBS}
        PkgConfig::TESSERACT
        PkgConfig::POPPLER
)

install(TARGETS docforge_engine DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/docforge)