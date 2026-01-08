
#include "boot_control.h"
#include "errors.h"

#include <cstdlib>
#include <iostream>

namespace up {

static std::string slot_to_string(Slot s) {
    switch (s) {
        case Slot::A: return "A";
        case Slot::B: return "B";
        default:      return "";
    }
}

void BootControl::switch_to(Slot target, const BootConfig& boot) {
    const std::string slot_str = slot_to_string(target);
    if (slot_str.empty()) {
        throw InstallError("BootControl: invalid slot");
    }

    // Prefer manifest override
    std::string cmd = boot.switch_cmd.has_value()
        ? *boot.switch_cmd
        : ("fw_setenv boot_slot " + slot_str);

    std::cerr << "[BootControl] switching boot to slot "
              << slot_str << " using: " << cmd << "\n";

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        throw InstallError("BootControl: failed to execute: " + cmd);
    }
}

} // namespace up
