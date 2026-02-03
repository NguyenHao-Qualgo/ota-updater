#include <gtest/gtest.h>

#include "flash/flasher.hpp"
#include "flash/io.hpp"
#include "flash/result.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace {

// In-memory reader (finite buffer)
class MemReader final : public flash::IReader {
public:
    explicit MemReader(std::vector<std::uint8_t> data)
        : data_(std::move(data)) {}

    ssize_t Read(std::span<std::uint8_t> out) override {
        if (pos_ >= data_.size()) return 0; // EOF
        const size_t n = std::min(out.size(), data_.size() - pos_);
        std::memcpy(out.data(), data_.data() + pos_, n);
        pos_ += n;
        return static_cast<ssize_t>(n);
    }

    std::optional<std::uint64_t> TotalSize() const override {
        return static_cast<std::uint64_t>(data_.size());
    }

private:
    std::vector<std::uint8_t> data_;
    size_t pos_{0};
};

// In-memory writer
class MemWriter final : public flash::IWriter {
public:
    flash::Result WriteAll(std::span<const std::uint8_t> in) override {
        out_.insert(out_.end(), in.begin(), in.end());
        return flash::Result::Ok();
    }

    flash::Result FsyncNow() override {
        // no-op for memory
        return flash::Result::Ok();
    }

    const std::vector<std::uint8_t>& Data() const { return out_; }

private:
    std::vector<std::uint8_t> out_;
};

TEST(FlasherTest, CopiesAllBytes) {
    // Arrange
    std::vector<std::uint8_t> input(3 * flash::kBlockSize + 123);
    for (size_t i = 0; i < input.size(); ++i) input[i] = static_cast<std::uint8_t>(i & 0xFF);

    MemReader reader(input);
    MemWriter writer;

    flash::FlashOptions opt{};
    opt.progress = false;
    opt.fsync_interval_bytes = 0; // disable fsync for test

    flash::Flasher flasher;

    // Act
    auto res = flasher.Run(reader, writer, opt);

    // Assert
    ASSERT_TRUE(res.ok) << res.msg;
    ASSERT_EQ(writer.Data().size(), input.size());
    ASSERT_EQ(writer.Data(), input);
}

TEST(FlasherTest, CallsFsyncByInterval) {
    // 3 blocks + 10 bytes
    std::vector<std::uint8_t> input(3 * flash::kBlockSize + 10, 0xAB);
    MemReader reader(input);

    class CountingWriter final : public flash::IWriter {
public:
        flash::Result WriteAll(std::span<const std::uint8_t> in) override {
            out_.insert(out_.end(), in.begin(), in.end());
            return flash::Result::Ok();
        }

        flash::Result FsyncNow() override {
            fsync_calls_++;
            return flash::Result::Ok();
        }

        size_t FsyncCalls() const { return fsync_calls_; }
        const std::vector<std::uint8_t>& Data() const { return out_; }

private:
        std::vector<std::uint8_t> out_;
        size_t fsync_calls_{0};
    } writer;

    flash::FlashOptions opt{};
    opt.progress = false;
    opt.fsync_interval_bytes = static_cast<std::uint64_t>(flash::kBlockSize); // fsync every ~1MiB

    flash::Flasher flasher;
    auto res = flasher.Run(reader, writer, opt);

    ASSERT_TRUE(res.ok) << res.msg;
    ASSERT_EQ(writer.Data().size(), input.size());

    // You fsync periodically AND also do a final fsync at end, so expect >= 3
    ASSERT_GE(writer.FsyncCalls(), 3u);
}

TEST(FlasherTest, ReturnsErrorWhenWriterFailsMidway) {
    // Large enough so we can fail in the middle
    std::vector<std::uint8_t> input(8 * flash::kBlockSize, 0x5A);
    MemReader reader(input);

    class FailingWriter final : public flash::IWriter {
public:
        explicit FailingWriter(size_t fail_after_bytes) : fail_after_(fail_after_bytes) {}

        flash::Result WriteAll(std::span<const std::uint8_t> in) override {
            if (written_ >= fail_after_) {
                return flash::Result::Fail(5, "Injected write failure");
            }
            // Write partially if this chunk crosses the boundary (simulate mid-write fail)
            const size_t can_write = std::min(in.size(), fail_after_ - written_);
            out_.insert(out_.end(), in.begin(), in.begin() + static_cast<std::ptrdiff_t>(can_write));
            written_ += can_write;

            if (can_write != in.size()) {
                return flash::Result::Fail(5, "Injected write failure");
            }
            return flash::Result::Ok();
        }

        flash::Result FsyncNow() override { return flash::Result::Ok(); }

        size_t Written() const { return written_; }
        const std::vector<std::uint8_t>& Data() const { return out_; }

private:
        size_t fail_after_{0};
        size_t written_{0};
        std::vector<std::uint8_t> out_;
    };

    // Fail after ~2.5 blocks
    FailingWriter writer(static_cast<size_t>(2 * flash::kBlockSize + flash::kBlockSize / 2));

    flash::FlashOptions opt{};
    opt.progress = false;
    opt.fsync_interval_bytes = 0;

    flash::Flasher flasher;
    auto res = flasher.Run(reader, writer, opt);

    ASSERT_FALSE(res.ok);
    ASSERT_LT(writer.Data().size(), input.size());
    ASSERT_GT(writer.Data().size(), 0u);
}

} // namespace
