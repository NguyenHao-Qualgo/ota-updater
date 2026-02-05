#include "flash/logger.hpp"

#include <cstdio>
#include <mutex>

namespace flash {

namespace {
std::mutex g_mu;
LogLevel g_level = LogLevel::Info;

const char* ToStr(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        default:              return "LOG";
    }
}
} // namespace

Logger& Logger::Instance() {
    static Logger inst;
    return inst;
}

void Logger::SetLevel(LogLevel lvl) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_level = lvl;
}

LogLevel Logger::Level() const {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_level;
}

void Logger::Log(LogLevel lvl, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    VLog(lvl, fmt, ap);
    va_end(ap);
}

void Logger::VLog(LogLevel lvl, const char* fmt, va_list ap) {
    LogLevel cur;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        cur = g_level;
    }

    if (lvl < cur) return;

    // Print to stderr (typical for tools)
    std::lock_guard<std::mutex> lk(g_mu);
    std::fprintf(stderr, "[%s] ", ToStr(lvl));
    std::vfprintf(stderr, fmt, ap);
    std::fprintf(stderr, "\n");
}

} // namespace flash
