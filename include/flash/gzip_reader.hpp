#pragma once

#include "flash/io.hpp"
#include <zlib.h>
#include <memory>
#include <vector>

namespace flash {

class GzipReader final : public IReader {
public:
    explicit GzipReader(std::unique_ptr<IReader> source);
    ~GzipReader() override;

    // Implementation of IReader
    ssize_t Read(std::span<std::uint8_t> out) override;
    std::optional<std::uint64_t> TotalSize() const override { return std::nullopt; }

private:
    std::unique_ptr<IReader> source_;
    z_stream strm_{};
    std::vector<std::uint8_t> in_buffer_;
    bool eof_reached_ = false;
};

} // namespace flash