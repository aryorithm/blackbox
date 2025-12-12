/**
 * @file performance_timer.h
 * @brief Scope-based Latency Profiler.
 *
 * Usage:
 * {
 *    PerformanceTimer t("AI_Inference", 5); // Warn if takes > 5ms
 *    brain->evaluate();
 * } // Destructor logs automatically if slow
 */

#ifndef BLACKBOX_COMMON_PERFORMANCE_TIMER_H
#define BLACKBOX_COMMON_PERFORMANCE_TIMER_H

#include <string>
#include <string_view>
#include <chrono>

namespace blackbox::common {

    class PerformanceTimer {
    public:
        /**
         * @brief Start the timer.
         *
         * @param name Name of the operation (e.g., "DB_Flush")
         * @param warn_threshold_ms Log warning if duration exceeds this (0 = always log)
         */
        PerformanceTimer(std::string_view name, double warn_threshold_ms = 10.0);

        /**
         * @brief Stop the timer and log if necessary.
         */
        ~PerformanceTimer();

    private:
        std::string name_;
        double threshold_ms_;
        std::chrono::steady_clock::time_point start_time_;
    };

} // namespace blackbox::common

#endif // BLACKBOX_COMMON_PERFORMANCE_TIMER_H