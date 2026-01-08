#include <gtest/gtest.h>
#include "UpdateManager.hpp"
#include "TegraSlotManager.hpp"

namespace {

class MockSlotManager : public up::ISlotManager {
public:
    up::Slot current = up::Slot::A;
    bool markSlotActive(up::Slot s) override { return true; }
    up::Slot getCurrentSlot() override { return current; }
};

class MockHandler : public up::IArtifactHandler {
public:
    bool called = false;
    bool handle(const up::InstallStep& s, up::ProgressCb c) override {
        called = true;
        return true;
    }
};

TEST(UpdateManagerTest, CorrectSlotBSelection) {
    auto slot = std::make_unique<MockSlotManager>();
    auto handler = std::make_shared<MockHandler>();
    
    auto testLogger = [](double progress, const std::string& msg) {
        std::cout << "[LOG] " << progress << "% : " << msg << std::endl;
    };

    up::UpdateManager manager(std::move(slot), testLogger);
    manager.registerHandler("raw", handler);

    up::Manifest m;
    m.slots["B"]["rootfs"] = "/dev/mmcblk0p4";
    m.items.push_back({"rootfs.img", "raw", "rootfs", std::nullopt});

    bool result = manager.runWithManifest(m);

    EXPECT_TRUE(result);
    EXPECT_TRUE(handler->called);
}

} // namespace