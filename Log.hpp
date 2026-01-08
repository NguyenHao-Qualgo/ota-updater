#pragma once
#include <print>
#include <string_view>
#include <format>

namespace up {

enum class LogLevel {
    INFO,
    VERBOSE,
    ERROR,
    SUCCESS
};

class Logger {
public:
    static inline bool verbose_enabled = false;

    template <typename... Args>
    static void info(std::string_view fmt, Args&&... args) {
        log(LogLevel::INFO, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void verbose(std::string_view fmt, Args&&... args) {
        if (verbose_enabled) {
            log(LogLevel::VERBOSE, fmt, std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    static void error(std::string_view fmt, Args&&... args) {
        log(LogLevel::ERROR, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void success(std::string_view fmt, Args&&... args) {
        log(LogLevel::SUCCESS, fmt, std::forward<Args>(args)...);
    }

private:
    template <typename... Args>
    static void log(LogLevel level, std::string_view fmt, Args&&... args) {
        std::string message = std::vformat(fmt, std::make_format_args(args...));

        switch (level) {
            case LogLevel::VERBOSE:
                std::println("\033[90m[DEBUG] {}\033[0m", message);
                break;
            case LogLevel::INFO:
                std::println("[INFO]  {}", message);
                break;
            case LogLevel::ERROR:
                std::println(stderr, "\033[31m[ERROR] {}\033[0m", message);
                break;
            case LogLevel::SUCCESS:
                std::println("\033[32m[ OK  ] {}\033[0m", message);
                break;
        }
    }
};

} // namespace up