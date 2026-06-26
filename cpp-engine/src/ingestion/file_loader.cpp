#include "file_loader.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <format>
#include <iostream>
#include <system_error>

namespace clearcapture {

// Supported file extensions
const std::vector<std::string> FileLoader::extensions_ = {
    ".pdf", ".png", ".jpg", ".jpeg", ".tiff", ".tif", ".bmp"
};

// MemoryMappedFile implementation

MemoryMappedFile::MemoryMappedFile(const fs::path& path)
    : path_(path)
{
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ == -1) {
        throw std::system_error(errno, std::generic_category(),
            std::format("failed to open file: {}", path.string()));
    }

    struct stat sb;
    if (::fstat(fd_, &sb) == -1) {
        ::close(fd_);
        fd_ = -1;
        throw std::system_error(errno, std::generic_category(),
            std::format("failed to stat file: {}", path.string()));
    }
    size_ = static_cast<size_t>(sb.st_size);

    if (size_ == 0) {
        data_ = nullptr;
        ::close(fd_);
        fd_ = -1;
        return;
    }

    void* mapped = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (mapped == MAP_FAILED) {
        ::close(fd_);
        fd_ = -1;
        throw std::system_error(errno, std::generic_category(),
            std::format("failed to mmap file: {}", path.string()));
    }

    data_ = static_cast<char*>(mapped);
    ::close(fd_);
    fd_ = -1;

    std::cout << std::format("[Capture] mmap'd {} ({} bytes)\n", path.string(), size_);
}

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept
    : path_(std::move(other.path_))
    , data_(other.data_)
    , size_(other.size_)
    , fd_(other.fd_)
{
    other.data_ = nullptr;
    other.size_ = 0;
    other.fd_ = -1;
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) noexcept {
    if (this != &other) {
        unmap();
        path_ = std::move(other.path_);
        data_ = other.data_;
        size_ = other.size_;
        fd_ = other.fd_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.fd_ = -1;
    }
    return *this;
}

MemoryMappedFile::~MemoryMappedFile() {
    unmap();
}

void MemoryMappedFile::unmap() {
    if (data_ != nullptr) {
        ::munmap(data_, size_);
        data_ = nullptr;
        size_ = 0;
    }
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

// FileLoader implementation

Result<FileLoader::LoadResult> FileLoader::load(const fs::path& path) {
    if (!fs::exists(path)) {
        return Result<LoadResult>(std::format("file not found: {}", path.string()));
    }

    if (!is_supported(path)) {
        return Result<LoadResult>(std::format("unsupported file extension: {}", path.extension().string()));
    }

    try {
        MemoryMappedFile mmf(path);
        return Result<LoadResult>(LoadResult{
            .buffer = mmf.get_span(),
            .filename = path.filename().string(),
            .extension = path.extension().string()
        });
    } catch (const std::exception& e) {
        return Result<LoadResult>(std::string(e.what()));
    }
}

bool FileLoader::is_supported(const fs::path& path) {
    std::string ext = path.extension().string();
    for (const auto& supported : extensions_) {
        if (ext == supported) return true;
    }
    return false;
}

const std::vector<std::string>& FileLoader::supported_extensions() {
    return extensions_;
}

} // namespace clearcapture