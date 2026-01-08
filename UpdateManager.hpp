#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>

namespace up {

enum class Slot { A, B, Unknown };
enum class ArtifactType { Raw, Archive, Script };

struct ArtifactItem {
    std::string file;
    std::string type;
    std::string target;
    std::optional<std::string> fstype;
};

struct Manifest {
    std::string version;
    std::map<std::string, std::map<std::string, std::string>> slots; 
    std::vector<ArtifactItem> items;
};

struct InstallStep {
    std::string source_file;
    std::string destination_device;
    ArtifactType type;
};

using ProgressCb = std::function<void(double, const std::string&)>;

class ISlotManager {
public:
    virtual ~ISlotManager() = default;
    virtual Slot getCurrentSlot() = 0;
    virtual bool markSlotActive(Slot slot) = 0;
};

class IArtifactHandler {
public:
    virtual ~IArtifactHandler() = default;
    virtual bool handle(const InstallStep& step, ProgressCb cb) = 0;
};

class UpdateManager {
public:
    UpdateManager(std::unique_ptr<ISlotManager> sm, ProgressCb cb);
    void registerHandler(const std::string& name, std::shared_ptr<IArtifactHandler> handler);
    
    bool run(const std::string& jsonPath);
    
    bool runWithManifest(const Manifest& manifest);

    std::vector<InstallStep> createPlan(const Manifest& manifest, Slot targetSlot);
    Manifest loadManifest(const std::string& path);

private:
    std::unique_ptr<ISlotManager> m_slotMgr;
    std::map<std::string, std::shared_ptr<IArtifactHandler>> m_handlers;
    ProgressCb m_logger;
};

} // namespace up