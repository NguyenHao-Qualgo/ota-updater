#include "slot_manager.h"
#include <iostream>

namespace up {

SlotInfo SlotManager::detect(const BootConfig& boot) {
    SlotInfo info;
    info.active = Slot::A;
    info.inactive = Slot::B;
    info.device_active = "/dev/mmcblk0p1";
    info.device_inactive = boot.inactive_slot_device.value_or("/dev/mmcblk0p2");
    std::cerr << "[stub] SlotManager: active=A, inactive=B\n";
    return info;
}

void SlotManager::mark_good(Slot /*s*/) {
    std::cerr << "[stub] SlotManager::mark_good\n";
}

void SlotManager::mark_bad(Slot /*s*/) {
    std::cerr << "[stub] SlotManager::mark_bad\n";
}

} // namespace up
