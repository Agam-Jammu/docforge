#pragma once

#include <span>
#include <string>
#include <vector>
#include <system_error>
#include <filesystem>
#include <format>
#include <memory>

namespace docforge {

namespace fs = std::filesystem;

/**
 * @brief Simple Result type (replaces std::expected which is C++23).
 */
template<typename T>
struct Result {
    T value;
    std::string error;
    bool ok = true;

    Result() = default;
    Result(T val) : value(std::move(val)), ok(true) {}
    Result(std::string err) : error(std::move(err)), ok(false) {}

    T& operator*() { return value; }
    const T& operator*() const { return value; }
    T* operator->() { return &value; }
    const T* operator->() const { return &value; }
    explicit operator bool() const { return ok; }
};

/**
 * @brief Memory-mapped file abstraction.
 *
 * Uses POSIX mmap to map file contents into the process address space
 * without copying into heap memory. This is the key "memory-mapped I/O"
 * talking point for interviews.
 *
 * On Windows, the equivalent is CreateFileMapping + MapViewOfFile.
 */
class MemoryMappedFile {
public:
    explicit MemoryMappedFile(const fs::path& path);

    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;
    MemoryMappedFile(MemoryMappedFile&& other) noexcept;
    MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept;

    ~MemoryMappedFile();

    [[nodiscard]] std::span<const char> get_span() const noexcept {
        return {data_, size_};
    }

    [[nodiscard]] const char* data() const noexcept { return data_; }
    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] const fs::path& path() const noexcept { return path_; }

private:
    void unmap();

    fs::path path_;
    char* data_ = nullptr;
    size_t size_ = 0;
    int fd_ = -1;
};

/**
 * @brief File ingestion engine: loads files using mmap.
 *
 * Files can be PDFs (handled via libpoppler rasterization) or
 * images (PNG, JPEG, TIFF handled via OpenCV imread).
 */
class FileLoader {
public:
    struct LoadResult {
        std::span<const char> buffer;
        std::string filename;
        std::string extension;
    };

    explicit FileLoader() = default;

    [[nodiscard]] Result<LoadResult> load(const fs::path& path);

    static bool is_supported(const fs::path& path);
    static const std::vector<std::string>& supported_extensions();

private:
    static const std::vector<std::string> extensions_;
};

} // namespace docforge