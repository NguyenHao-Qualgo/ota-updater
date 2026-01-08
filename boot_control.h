
#pragma once
#include "types.h"
#include <string>

namespace up {

/**
 * BootControl
 *
 * Minimal boot target control abstraction.
 * - Switches boot target to the requested slot.
 * - Uses BootConfig.switch_cmd if provided (e.g., "fw_setenv boot_slot B").
 * - Otherwise defaults to U-Boot style: "fw_setenv boot_slot <A|B>".
 */
class BootControl {
public:
    /**
     * Switch boot target to the given slot using BootConfig.
     * @throws InstallError on invalid slot or command failure
     */
    void switch_to(Slot target, const BootConfig& boot);
};

} // namespace up
