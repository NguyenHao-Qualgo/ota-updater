#include <gtest/gtest.h>
#include "flash/manifest.hpp"

using namespace flash;

TEST(ManifestTest, VersionComparisonLogic) {
    // Standard cases
    EXPECT_EQ(ManifestHandler::CompareVersions("1.2.3", "1.2.2"), 1);
    EXPECT_EQ(ManifestHandler::CompareVersions("1.0.0", "2.0.0"), -1);
    
    EXPECT_EQ(ManifestHandler::CompareVersions("1.2", "1.2.0"), 0);
    EXPECT_EQ(ManifestHandler::CompareVersions("1.2.1", "1.2"), 1);
    EXPECT_EQ(ManifestHandler::CompareVersions("2", "1.9.9"), 1);

    EXPECT_EQ(ManifestHandler::CompareVersions("1.2.a", "1.2.0"), 0);
    EXPECT_EQ(ManifestHandler::CompareVersions("", ""), 0);
}

TEST(ManifestTest, ShouldUpdateLogic) {
    Manifest m;
    m.hw_compatibility = "orin-nano";
    
    Component c;
    c.name = "kernel";
    c.version = "2.0.0";

    EXPECT_TRUE(ManifestHandler::ShouldUpdate(c, m, "1.9.0"));

    EXPECT_FALSE(ManifestHandler::ShouldUpdate(c, m, "2.1.0"));

    EXPECT_FALSE(ManifestHandler::ShouldUpdate(c, m, "2.0.0"));

    c.force = true;
    EXPECT_TRUE(ManifestHandler::ShouldUpdate(c, m, "2.1.0"));
}

TEST(ManifestTest, ParserEdgeCases) {
    std::string bad_json = R"({"something": "invalid"})";
    auto res1 = ManifestHandler::Parse(bad_json);
    EXPECT_TRUE(res1->components.empty());

    std::string garbage = "not json at all";
    auto res2 = ManifestHandler::Parse(garbage);
    EXPECT_FALSE(res2.has_value());

    std::string type_mismatch = R"({
        "version": 1.0, 
        "components": [{"name": "test", "version": 2}]
    })";
    auto res3 = ManifestHandler::Parse(type_mismatch);
    EXPECT_FALSE(res3.has_value());
}

TEST(ManifestTest, PlanningLogicWithForce) {
    Manifest m;
    m.force_all = true;
    Component c;
    c.version = "0.0.1"; // Older than system

    EXPECT_TRUE(ManifestHandler::ShouldUpdate(c, m, "1.0.0"));
    m.force_all = false;
    EXPECT_FALSE(ManifestHandler::ShouldUpdate(c, m, "1.0.0"));
}

TEST(ManifestTest, FullParseTest) {
    std::string raw = R"({"version":"1.0","components":[{"name":"boot","version":"1.1"}]})";
    auto m = ManifestHandler::Parse(raw);
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->components[0].version, "1.1");
}