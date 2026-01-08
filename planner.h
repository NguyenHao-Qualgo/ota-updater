#pragma once
#include "types.h"
#include <vector>

namespace up {

struct InstallStep {
    std::string name;
    Artifact artifact;
};

struct InstallPlan {
    std::vector<InstallStep> steps;
};

class Planner {
public:
    InstallPlan build(const Manifest& m);
};

} // namespace up
