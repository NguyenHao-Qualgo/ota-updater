#include "health_check.h"

namespace up {

void HealthCheck::add(Check c) { checks_.push_back(std::move(c)); }

bool HealthCheck::run_all(ProgressCb progress) {
    bool ok = true;
    for (auto& c : checks_) {
        bool r = c.fn ? c.fn() : true;
        if (progress) progress(-1.0, std::string("check: ") + c.description + (r ? " [OK]" : " [FAIL]"));
        ok = ok && r;
    }
    // If there were no checks, still report OK
    if (checks_.empty() && progress) progress(-1.0, "health_check: no checks, assuming OK");
    return ok;
}

} // namespace up
