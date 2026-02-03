// partition_writer.cpp - Writer implementation for block device/partition path.

#include "flash/partition_writer.hpp"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

namespace flash {

Result PartitionWriter::Open(std::string path, PartitionWriter &out) {
    out.path_ = std::move(path);

    // O_WRONLY is enough for MVP; later you can add O_SYNC / O_DIRECT options.
    int fd = ::open(out.path_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return Result::Fail(
            errno,
            "Failed to open output: " + out.path_ + " (" + std::strerror(errno) + ")");
    }
    out.fd_.Reset(fd);
    return Result::Ok();
}

Result PartitionWriter::WriteAll(std::span<const std::uint8_t> in) {
    size_t rem = in.size();
    const std::uint8_t *p = in.data();

    while (rem > 0) {
        ssize_t n = ::write(fd_.Get(), p, rem);
        if (n > 0) {
            p += static_cast<size_t>(n);
            rem -= static_cast<size_t>(n);
            continue;
        }
        if (n == -1 && errno == EINTR) {
            continue;
        }
        return Result::Fail(errno, "Write failed (" + std::string(std::strerror(errno)) + ")");
    }

    return Result::Ok();
}

Result PartitionWriter::FsyncNow() {
    if (::fsync(fd_.Get()) == -1) {
        return Result::Fail(errno, "fsync failed (" + std::string(std::strerror(errno)) + ")");
    }
    return Result::Ok();
}

} // namespace flash
