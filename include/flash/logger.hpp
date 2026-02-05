#pragma once

#include <cstdarg>
#include <string>

namespace flash {

enum class LogLevel : int {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
    None  = 4,
};

class Logger {
public:
    static Logger& Instance();

    void SetLevel(LogLevel lvl);
    LogLevel Level() const;

    // printf-style logging
    void Log(LogLevel lvl, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
    void VLog(LogLevel lvl, const char* fmt, va_list ap);

private:
    Logger() = default;
};

inline void LogDebug(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
inline void LogInfo (const char* fmt, ...) __attribute__((format(printf, 1, 2)));
inline void LogWarn (const char* fmt, ...) __attribute__((format(printf, 1, 2)));
inline void LogError(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

inline void LogDebug(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    Logger::Instance().VLog(LogLevel::Debug, fmt, ap);
    va_end(ap);
}
inline void LogInfo(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    Logger::Instance().VLog(LogLevel::Info, fmt, ap);
    va_end(ap);
}
inline void LogWarn(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    Logger::Instance().VLog(LogLevel::Warn, fmt, ap);
    va_end(ap);
}
inline void LogError(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    Logger::Instance().VLog(LogLevel::Error, fmt, ap);
    va_end(ap);
}

} // namespace flash
