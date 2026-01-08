#include "UpdateManager.hpp"
#include "Log.hpp"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace up {

UpdateManager::UpdateManager(std::unique_ptr<ISlotManager> sm, ProgressCb cb)
    : m_slotMgr(std::move(sm)), m_logger(cb) {}

void UpdateManager::registerHandler(const std::string& name, std::shared_ptr<IArtifactHandler> handler) {
    Logger::verbose("Registering handler for type: {}", name);
    m_handlers[name] = handler;
}

std::vector<InstallStep> UpdateManager::createPlan(const Manifest& manifest, Slot targetSlot) {
    std::vector<InstallStep> plan;
    std::string slotKey = (targetSlot == Slot::A) ? "A" : "B";
    
    Logger::verbose("Creating installation plan for Slot {}", slotKey);

    if (manifest.slots.find(slotKey) == manifest.slots.end()) {
        throw std::runtime_error("Manifest 'slots' node is missing key: " + slotKey);
    }

    const auto& targetMap = manifest.slots.at(slotKey);

    for (const auto& item : manifest.items) {
        InstallStep step;
        step.source_file = item.file;
        
        if (targetMap.find(item.target) == targetMap.end()) {
            throw std::runtime_error("Slot " + slotKey + " missing target: " + item.target);
        }
        
        step.destination_device = targetMap.at(item.target);
        step.type = (item.type == "archive") ? ArtifactType::Archive : ArtifactType::Raw;
        
        Logger::verbose("  - Step: {} -> {}", step.source_file, step.destination_device);
        plan.push_back(step);
    }
    return plan;
}

bool UpdateManager::run(const std::string& jsonPath) {
    try {
        Logger::info("Loading manifest: {}", jsonPath);
        Manifest manifest = loadManifest(jsonPath);
        return runWithManifest(manifest);
    } catch (const std::exception& e) {
        Logger::error("Update failed: {}", e.what());
        m_logger(-1, e.what()); // Signal UI/CLI of fatal error
        return false;
    }
}

bool UpdateManager::runWithManifest(const Manifest& manifest) {
    Logger::info("Update started (Version: {})", manifest.version);

    // 1. Slot Detection
    Slot current = m_slotMgr->getCurrentSlot();
    Slot target = (current == Slot::A) ? Slot::B : Slot::A;
    
    Logger::info("Detected current slot: {}", (current == Slot::A ? "A" : "B"));
    Logger::info("Target installation slot: {}", (target == Slot::A ? "A" : "B"));

    // 2. Planning
    auto plan = createPlan(manifest, target);
    m_logger(10, "Planning complete");

    // 3. Execution
    for (size_t i = 0; i < plan.size(); ++i) {
        const auto& step = plan[i];
        
        // Progress calculation
        double baseProgress = 10.0 + (i * (80.0 / plan.size()));
        m_logger(baseProgress, "Installing " + step.source_file);
        
        Logger::info("Installing artifact {}/{} ({})", i + 1, plan.size(), step.source_file);

        std::string handlerKey = (step.type == ArtifactType::Archive) ? "archive" : "raw";
        
        if (m_handlers.count(handlerKey)) {
            if (!m_handlers[handlerKey]->handle(step, m_logger)) {
                throw std::runtime_error("Handler '" + handlerKey + "' failed for " + step.source_file);
            }
        } else {
            throw std::runtime_error("No handler registered for type: " + handlerKey);
        }
    }

    // 4. Finalization
    m_logger(95, "Finalizing boot slot...");
    Logger::info("Switching active boot slot to {}...", (target == Slot::A ? "A" : "B"));
    
    if (m_slotMgr->markSlotActive(target)) {
        m_logger(100, "Update finished successfully");
        Logger::success("Slot switch completed successfully.");
        return true;
    }

    Logger::error("Failed to switch boot slot!");
    return false;
}

Manifest UpdateManager::loadManifest(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Manifest file not found: " + path);

    nlohmann::json j;
    file >> j;

    Manifest m;
    m.version = j.value("version", "1.0.0");
    
    Logger::verbose("Parsing JSON slot map...");
    if (j.contains("slots")) {
        for (auto& [slotName, partitionMap] : j["slots"].items()) {
            for (auto& [targetName, p] : partitionMap.items()) {
                m.slots[slotName][targetName] = p.get<std::string>();
            }
        }
    }

    Logger::verbose("Parsing JSON artifact list...");
    if (j.contains("artifacts")) {
        for (auto& group : j["artifacts"]) {
            if (!group.contains("items")) continue;
            for (auto& item : group["items"]) {
                m.items.push_back({
                    item.value("file", ""), 
                    item.value("type", "raw"), 
                    item.value("target", ""),
                    std::nullopt
                });
            }
        }
    }

    Logger::verbose("Manifest loaded: {} artifacts found", m.items.size());
    return m;
}

} // namespace up