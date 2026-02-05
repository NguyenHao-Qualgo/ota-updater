#pragma once
#include <cstdint>
#include <string_view>

namespace flash {

struct ProgressEvent {
    std::string_view component;
    std::uint64_t comp_done = 0;
    std::uint64_t comp_total = 0;

    std::uint64_t overall_done = 0;
    std::uint64_t overall_total = 0;
};

class IProgress {
public:
    virtual ~IProgress() = default;
    virtual void OnProgress(const ProgressEvent& e) = 0;
};

} // namespace flash
