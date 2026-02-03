#include <gtest/gtest.h>

#include "flash/partition_writer.hpp"
#include "flash/result.hpp"

#include "testing.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace {

class PartitionWriterTests : public ::testing::Test {
protected:
    testutil::TemporaryDirectory tmp;

    std::string MakePath(const std::string &name) {
        return tmp.Path() + "/" + name;
    }

    std::vector<std::uint8_t> ReadFile(const std::string &path) {
        std::ifstream is(path, std::ios::binary);
        EXPECT_TRUE(is.good());
        is.seekg(0, std::ios::end);
        const auto size = static_cast<size_t>(is.tellg());
        is.seekg(0, std::ios::beg);
        std::vector<std::uint8_t> data(size);
        is.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        return data;
    }
};

TEST_F(PartitionWriterTests, OpenNonexistentPath_Fails) {
    // Directory doesn't exist -> open should fail
    flash::PartitionWriter w;
    auto res = flash::PartitionWriter::Open(tmp.Path() + "/no_such_dir/out.bin", w);
    ASSERT_FALSE(res.ok);
}

TEST_F(PartitionWriterTests, WriteAll_WritesExactBytes) {
    const std::string out_path = MakePath("out.bin");

    flash::PartitionWriter w;
    auto res = flash::PartitionWriter::Open(out_path, w);
    ASSERT_TRUE(res.ok) << res.msg;

    std::vector<std::uint8_t> data(1024 * 1024 + 123);
    for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<std::uint8_t>((i ^ 0x5A) & 0xFF);

    auto wr = w.WriteAll(std::span<const std::uint8_t>(data.data(), data.size()));
    ASSERT_TRUE(wr.ok) << wr.msg;

    auto fs = w.FsyncNow();
    ASSERT_TRUE(fs.ok) << fs.msg;

    // Read back and compare
    auto read_back = ReadFile(out_path);
    EXPECT_EQ(read_back, data);
}

} // namespace
