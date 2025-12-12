/**
 * @file time_utils.h
 * @brief High-Performance Time Manipulation.
 *
 * Optimized timestamp formatting to avoid the overhead of
 * std::stringstream and std::locale in hot paths.
 */

#ifndef BLACKBOX_COMMON_TIME_UTILS_H
#define BLACKBOX_COMMON_TIME_UTILS_H

#include <string>
#include <cstdint>
#include <chrono>

namespace blackbox::common {

    class TimeUtils {
    public:
        /**
         * @brief Get current system time in Nanoseconds.
         * Useful for precise latency tracking.
         */
        static uint64_t now_ns();

        /**
         * @brief Get current system time in Milliseconds.
         * Useful for database timestamps.
         */
        static uint64_t now_ms();

        /**
         * @brief Fast conversion of epoch ms to "YYYY-MM-DD HH:MM:SS".
         *
         * This format is required by ClickHouse 'DateTime' columns.
         * Optimized to avoid heavy heap allocations.
         *
         * @param timestamp_ms Epoch milliseconds
         * @return Formatted string
         */
        static std::string to_clickhouse_format(uint64_t timestamp_ms);

        /**
         * @brief Fast conversion to ISO 8601 "YYYY-MM-DDTHH:MM:SS.mmmZ".
         * Used for JSON logs.
         */
        static std::string to_iso_8601(uint64_t timestamp_ms);

        /**
         * @brief Parses standard Syslog date "MMM dd HH:mm:ss".
         * e.g., "Dec 12 10:00:00" -> Epoch Seconds.
         *
         * Handles year inference (Syslog doesn't include year).
         */
        static uint64_t parse_syslog_time(const std::string& date_str);
    };

} // namespace blackbox::common

#endif // BLACKBOX_COMMON_TIME_UTILS_H