#pragma once
#include "types.h"
#include <string>
#include <optional>

namespace up {

enum class Step {
    None,
    Verifying,
    WritingArtifact,
    PostInstallChecks,
    SwitchingBoot,
    Completed
};

struct UpdateState {
    std::string version;
    Step current_step = Step::None;
    std::optional<std::string> current_artifact;
    uint64_t bytes_written = 0;     // optional for resume
    std::string target;
};

class Journal {
public:
    explicit Journal(const std::string& path = "/var/lib/updater/state.json");
    UpdateState load();
    void save(const UpdateState& s);
    void clear();
private:
    std::string path_;
};

} // namespace up
