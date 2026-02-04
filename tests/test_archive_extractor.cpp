#include <gtest/gtest.h>
#include "flash/archive_extractor.hpp"
#include "testing.hpp"
#include <fstream>

namespace flash {

class ArchiveExtractorTest : public ::testing::Test {
protected:
    testutil::TemporaryDirectory temp_dir;

    std::string CreateMockPackage() {
        std::string pkg_path = temp_dir.Path() + "/update.tar";
        return pkg_path;
    }
};

TEST_F(ArchiveExtractorTest, FailsIfManifestMissing) {
    ArchiveExtractor extractor;
    std::ofstream dummy(temp_dir.Path() + "/not_a_tar.txt");
    dummy << "hello";
    dummy.close();

    auto res = extractor.ProcessPackage(temp_dir.Path() + "/not_a_tar.txt");
    EXPECT_FALSE(res.is_ok());
}

} // namespace flash