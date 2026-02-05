#pragma once
#include "flash/io.hpp"
#include <cstdint>
#include <memory>

namespace flash {

class CountingReader final : public IReader {
public:
    explicit CountingReader(std::unique_ptr<IReader> inner)
        : inner_(std::move(inner)) {}

    ssize_t Read(std::span<std::uint8_t> out) override {
        const ssize_t n = inner_->Read(out);
        if (n > 0) read_ += (std::uint64_t)n;
        return n;
    }

    std::uint64_t BytesRead() const { return read_; }

    std::optional<std::uint64_t> TotalSize() const override {
        return inner_ ? inner_->TotalSize() : std::nullopt;
    }

private:
    std::unique_ptr<IReader> inner_;
    std::uint64_t read_ = 0;
};

} // namespace flash
