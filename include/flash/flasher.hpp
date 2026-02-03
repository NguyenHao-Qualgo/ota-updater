#pragma once

#include "flash/io.hpp"
#include "flash/result.hpp"

#include <cstdint>
#include <optional>
#include <time.h>

namespace flash {

inline constexpr size_t kBlockSize = 1024 * 1024; // 1 MiB

[[nodiscard]] inline std::uint64_t NowMs() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000ULL
         + static_cast<std::uint64_t>(ts.tv_nsec) / 1'000'000ULL;
}

struct FlashOptions {
    std::uint64_t fsync_interval_bytes{static_cast<std::uint64_t>(kBlockSize)};
    bool progress{false};
};

class Flasher {
public:
    Result Run(IReader &reader, IWriter &writer, const FlashOptions &opt);
};

} // namespace flash
