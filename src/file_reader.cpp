#include "flash/file_reader.hpp"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace flash {

Result FileOrStdinReader::Open(std::string path, FileOrStdinReader &out) {
    out.path_ = std::move(path);

    if (out.path_ == "-") {
        out.fd_.Reset(STDIN_FILENO);
        out.size_ = std::nullopt;
        return Result::Ok();
    }

    int fd = ::open(out.path_.c_str(), O_RDONLY);
    if (fd < 0) {
        return Result::Fail(
            errno,
            "Failed to open input: " + out.path_ + " (" + std::strerror(errno) + ")");
    }
    out.fd_.Reset(fd);

    struct stat st {};
    if (::fstat(fd, &st) == 0 && st.st_size > 0) {
        out.size_ = static_cast<std::uint64_t>(st.st_size);
    } else {
        out.size_ = std::nullopt;
    }

    return Result::Ok();
}

std::optional<std::uint64_t> FileOrStdinReader::TotalSize() const {
    return size_;
}

ssize_t FileOrStdinReader::Read(std::span<std::uint8_t> out) {
    while (true) {
        ssize_t n = ::read(fd_.Get(), out.data(), out.size());
        if (n >= 0) {
            return n;
        }
        if (errno == EINTR) {
            continue;
        }
        return -1;
    }
}

} // namespace flash
