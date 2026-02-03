#pragma once
#include <atomic>

namespace flash {

extern std::atomic_bool g_cancel;

void InstallSignalHandlers();

} // namespace flash
