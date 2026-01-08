#pragma once
#include "UpdateManager.hpp"
#include "Log.hpp"
#include <array>
#include <memory>
#include <cstdio>

namespace up {

class TegraSlotManager : public ISlotManager {
public:
    Slot getCurrentSlot() override {
        auto output = exec("nvbootctrl get-current-slot");
        if (output.find("0") != std::string::npos) return Slot::A;
        if (output.find("1") != std::string::npos) return Slot::B;
        
        Logger::error("Failed to detect current slot from nvbootctrl");
        return Slot::Unknown;
    }

    bool markSlotActive(Slot slot) override {
        int idx = (slot == Slot::A) ? 0 : 1;
        Logger::verbose("Executing: nvbootctrl set-active-boot-slot {}", idx);
        
        std::string cmd = "nvbootctrl set-active-boot-slot " + std::to_string(idx);
        return (std::system(cmd.c_str()) == 0);
    }

private:
    struct PcloseDeleter {
        void operator()(FILE* f) const {
            if (f) pclose(f);
        }
    };

    std::string exec(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;

        std::unique_ptr<FILE, PcloseDeleter> pipe(popen(cmd, "r"));

        if (!pipe) {
            Logger::error("popen() failed!");
            return "";
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }
};

} // namespace up