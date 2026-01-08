#include "planner.h"

namespace up {

InstallPlan Planner::build(const Manifest& m) {
    InstallPlan plan;
    for (const auto& a : m.artifacts) {
        plan.steps.push_back({ "Install artifact", a });
    }
    return plan;
}

} // namespace up