#include <gtest/gtest.h>

#include "flash/file_reader.hpp"
#include "flash/result.hpp"

#include "testing.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

class FileReaderTests : public ::testing::Test {
protected:
    testutil::TemporaryDirectory tmp;

    std::string MakePath(const std::string &name) {
        return tmp.Path() + "/" + name;
    }

    void WriteFile(const std::string &path, const std::vector<std::uint8_t> &data) {
        std::ofstream os(path, std::ios::binary);
        ASSERT_TRUE(os.good());
        os.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        os.close();
        ASSERT_TRUE(os.good());
    }
};

TEST_F(FileReaderTests, OpenOK_AndTotalSizeMatches) {
    const std::string p = MakePath("in.bin");
    std::vector<std::uint8_t> data(12345);
    for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<std::uint8_t>(i & 0xFF);
    WriteFile(p, data);

    flash::FileOrStdinReader r;
    auto res = flash::FileOrStdinReader::Open(p, r);
    ASSERT_TRUE(res.ok) << res.msg;

    auto sz = r.TotalSize();
    ASSERT_TRUE(sz.has_value());
    EXPECT_EQ(*sz, data.size());
}

TEST_F(FileReaderTests, OpenNonexistent_Fails) {
    flash::FileOrStdinReader r;
    auto res = flash::FileOrStdinReader::Open(MakePath("nope.bin"), r);
    ASSERT_FALSE(res.ok);
}

TEST_F(FileReaderTests, ReadAllBytes_EqualsInput) {
    const std::string p = MakePath("in2.bin");
    std::vector<std::uint8_t> data(2 * 1024 * 1024 + 7);
    for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<std::uint8_t>((i * 13) & 0xFF);
    WriteFile(p, data);

    flash::FileOrStdinReader r;
    auto res = flash::FileOrStdinReader::Open(p, r);
    ASSERT_TRUE(res.ok) << res.msg;

    std::vector<std::uint8_t> out;
    out.resize(data.size());

    size_t pos = 0;
    while (pos < out.size()) {
        std::span<std::uint8_t> buf(out.data() + pos, out.size() - pos);
        ssize_t n = r.Read(buf);
        ASSERT_GE(n, 0);
        if (n == 0) break;
        pos += static_cast<size_t>(n);
    }

    ASSERT_EQ(pos, data.size());
    EXPECT_EQ(out, data);
}

} // namespace
