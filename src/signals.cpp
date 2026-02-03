// signals.cpp - Signal handling and shared cancel flag.

#include "flash/signals.hpp"

#include <csignal>

namespace flash {

std::atomic_bool g_cancel{false};

static void HandleSignal(int) {
    g_cancel.store(true, std::memory_order_relaxed);
}

void InstallSignalHandlers() {
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);
}

} // namespace flash
