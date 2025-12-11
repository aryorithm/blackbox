/**
 * @file logger.cpp
 * @brief Implementation of Thread-Safe Logging.
 */

#include "blackbox/common/logger.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace blackbox::common {

    // ANSI Color Codes
    constexpr const char* RESET   = "\033[0m";
    constexpr const char* RED     = "\033[31m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* BLUE    = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN    = "\033[36m";

    Logger& Logger::instance() {
        static Logger instance;
        return instance;
    }

    void Logger::set_level(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        min_level_ = level;
    }

    void Logger::log(LogLevel level, std::string_view message, const char* file, int line) {
        // Double-check locking (optimization)
        if (level < min_level_) return;

        std::lock_guard<std::mutex> lock(mutex_);

        // 1. Get Current Time
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        // 2. Select Color & Label
        const char* color = RESET;
        const char* label = "[INFO]";
        
        switch (level) {
            case LogLevel::DEBUG:    color = CYAN;    label = "[DEBUG]"; break;
            case LogLevel::INFO:     color = GREEN;   label = "[INFO] "; break; // Space for alignment
            case LogLevel::WARN:     color = YELLOW;  label = "[WARN] "; break;
            case LogLevel::ERROR:    color = RED;     label = "[ERROR]"; break;
            case LogLevel::CRITICAL: color = MAGENTA; label = "[CRIT] "; break;
        }

        // 3. Format Output
        // [TIME] [LEVEL] Message (File:Line)
        std::cout << color << "[" 
                  << std::put_time(std::localtime(&time_t_now), "%H:%M:%S") 
                  << "." << std::setfill('0') << std::setw(3) << ms.count() 
                  << "] " << label << " " 
                  << message 
                  << RESET;

        // Add file info only for errors/debug to keep logs clean
        if (level == LogLevel::ERROR || level == LogLevel::DEBUG || level == LogLevel::CRITICAL) {
             std::cout << " (" << file << ":" << line << ")";
        }

        std::cout << std::endl;
    }

} // namespace blackbox::common