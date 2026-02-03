#pragma once

#include "flash/fd.hpp"
#include "flash/io.hpp"
#include "flash/result.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace flash {

class FileOrStdinReader final : public IReader {
public:
    static Result Open(std::string path, FileOrStdinReader &out);

    std::optional<std::uint64_t> TotalSize() const override;
    ssize_t Read(std::span<std::uint8_t> out) override;

private:
    std::string path_;
    Fd fd_;
    std::optional<std::uint64_t> size_;
};

} // namespace flash
