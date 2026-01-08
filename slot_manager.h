#pragma once
#include "types.h"
#include <string>

namespace up {

struct SlotInfo {
    Slot active = Slot::Unknown;
    Slot inactive = Slot::Unknown;
    std::string device_active;
    std::string device_inactive;
};

class SlotManager {
public:
    SlotInfo detect(const BootConfig& boot);
    void mark_good(Slot s);
    void mark_bad(Slot s);
};


} // namespace up
