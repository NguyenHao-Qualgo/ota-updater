#pragma once
#include <string>
#include <vector>
#include "types.h"

namespace up {

struct Check {
    std::string description;
    std::function<bool(void)> fn;
};

class HealthCheck {
public:
    void add(Check c);
    bool run_all(ProgressCb progress);
private:
    std::vector<Check> checks_;
};

} // namespace up
