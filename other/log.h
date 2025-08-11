#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <string>

enum class LogLevel {
    INFO_L,
    WARN_L,
    ERROR_L
};

inline void log(const LogLevel level, const std::string& message) {
    const char* prefix = nullptr;
    switch (level) {
        case LogLevel::INFO_L:  prefix = "[INFO] "; break;
        case LogLevel::WARN_L:  prefix = "[WARN] "; break;
        case LogLevel::ERROR_L: prefix = "[ERROR] "; break;
    }

    std::ostream& os = (level == LogLevel::ERROR_L) ? std::cerr : std::cout;
    os << prefix << message << std::endl;
}

#define LOG_INFO(msg)  log(LogLevel::INFO_L, msg)
#define LOG_WARN(msg)  log(LogLevel::WARN_L, msg)
#define LOG_ERROR(msg) log(LogLevel::ERROR_L, msg)

#endif //LOG_H
