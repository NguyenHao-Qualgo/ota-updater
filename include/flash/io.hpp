#pragma once

#include "flash/result.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <sys/types.h>

namespace flash {

class IReader {
public:
    virtual ~IReader() = default;
    virtual ssize_t Read(std::span<std::uint8_t> out) = 0;
    virtual std::optional<std::uint64_t> TotalSize() const { return std::nullopt; }
};

class IWriter {
public:
    virtual ~IWriter() = default;
    virtual Result WriteAll(std::span<const std::uint8_t> in) = 0;
    virtual Result FsyncNow() = 0;
};

} // namespace flash
