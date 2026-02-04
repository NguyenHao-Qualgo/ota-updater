#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "flash/gzip_reader.hpp"

namespace flash {

class BufferReader : public IReader {
public:
    explicit BufferReader(std::vector<uint8_t> data) : data_(std::move(data)) {}
    
    ssize_t Read(std::span<std::uint8_t> out) override {
        if (pos_ >= data_.size()) return 0;
        size_t to_read = std::min(out.size(), data_.size() - pos_);
        std::copy(data_.begin() + pos_, data_.begin() + pos_ + to_read, out.begin());
        pos_ += to_read;
        return static_cast<ssize_t>(to_read);
    }
private:
    std::vector<uint8_t> data_;
    size_t pos_ = 0;
};

TEST(GzipReaderTest, SuccessfullyDecompressesGzipData) {
    // echo -n "hello" | gzip -c | xxd -i
    std::vector<uint8_t> compressed_data = {
        0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xcb, 0x48,
        0xcd, 0xc9, 0xc9, 0x07, 0x00, 0x86, 0xa6, 0x10, 0x36, 0x05, 0x00, 0x00, 0x00
    };

    auto base_reader = std::make_unique<BufferReader>(compressed_data);
    GzipReader gz_reader(std::move(base_reader));

    std::vector<uint8_t> output_buffer(64, 0);
    ssize_t n = gz_reader.Read(output_buffer);

    ASSERT_GT(n, 0);
    
    std::string decompressed(reinterpret_cast<char*>(output_buffer.data()), n);
    
    // Change this line to match the actual data
    EXPECT_EQ(decompressed, "hello"); 
}

TEST(GzipReaderTest, HandlesInvalidGzipHeader) {
    std::vector<uint8_t> garbage = {0x00, 0x01, 0x02, 0x03};
    
    auto base_reader = std::make_unique<BufferReader>(garbage);
    GzipReader gz_reader(std::move(base_reader));

    std::vector<uint8_t> output_buffer(64);
    ssize_t n = gz_reader.Read(output_buffer);

    EXPECT_LT(n, 0);
}

}