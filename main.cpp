#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <vector>
#include <string_view>

#include "Log.hpp"
#include "UpdateManager.hpp"
#include "TegraSlotManager.hpp"
#include "RawImageHandler.cpp"

namespace fs = std::filesystem;
using namespace up;

int main(int argc, char* argv[]) {
    // 1. Convert arguments to string_view for modern C++23 parsing
    std::vector<std::string_view> args(argv + 1, argv + argc);
    std::string packagePath;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "-v" || args[i] == "--verbose") {
            Logger::verbose_enabled = true;
        } else if (packagePath.empty()) {
            packagePath = args[i];
        }
    }

    if (packagePath.empty()) {
        Logger::error("No update package provided!");
        std::println("Usage: tegra-updater <update_package.zip> [-v|--verbose]");
        return EXIT_FAILURE;
    }

    std::string stagingDir = "/tmp/ota_extract";

    // 2. Define a bridge between the Progress Callback and our new Logger
    auto otaCallback = [](double p, const std::string& msg) {
        if (p < 0) {
            Logger::error("{}", msg);
        } else if (p >= 100.0) {
            Logger::success("{}", msg);
        } else {
            // Using C++23 std::print for a smooth progress bar
            std::print("\r\033[36m[PROGRESS]\033[0m {:3.0f}% | {}", p, msg);
            std::fflush(stdout);
        }
    };

    try {
        Logger::info("Tegra OTA Update Initialized");
        Logger::verbose("Package: {}", packagePath);
        Logger::verbose("Staging Directory: {}", stagingDir);

        // 3. Prepare Staging Area
        if (fs::exists(stagingDir)) {
            Logger::verbose("Cleaning old staging directory...");
            fs::remove_all(stagingDir);
        }
        fs::create_directories(stagingDir);

        // 4. Extract Package
        Logger::info("Extracting update package...");
        std::string cmd = std::format("unzip -q {} -d {}", packagePath, stagingDir);
        
        if (std::system(cmd.c_str()) != 0) {
            throw std::runtime_error("Failed to extract ZIP package. Is 'unzip' installed?");
        }

        std::string manifestPath = stagingDir + "/manifest.json";
        if (!fs::exists(manifestPath)) {
            throw std::runtime_error("Invalid package: manifest.json not found inside zip.");
        }

        // 5. Initialize Update Logic
        auto slotMgr = std::make_unique<TegraSlotManager>();
        UpdateManager manager(std::move(slotMgr), otaCallback);

        auto rawHandler = std::make_shared<RawImageHandler>();
        manager.registerHandler("raw", rawHandler);

        // Switch context to extracted folder so relative paths in JSON resolve correctly
        fs::current_path(stagingDir);
        Logger::verbose("Switched working directory to: {}", stagingDir);

        // 6. Execute Update
        if (manager.run("manifest.json")) {
            std::println(""); // New line after progress bar
            Logger::success("Update applied successfully.");
            
            Logger::verbose("Cleaning up staging files...");
            fs::remove_all(stagingDir);
            
            Logger::info("System is ready for reboot.");
            return EXIT_SUCCESS;
        }

    } catch (const std::exception& e) {
        std::println(""); // Ensure error starts on new line
        Logger::error("Update aborted: {}", e.what());
    }

    return EXIT_FAILURE;
}