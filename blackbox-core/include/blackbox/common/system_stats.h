/**
 * @file system_stats.h
 * @brief Resource Usage Monitor (Linux /proc interface).
 * 
 * Reads process-level CPU and Memory usage.
 * Essential for Kubernetes Liveness probes and Grafana dashboards.
 */

#ifndef BLACKBOX_COMMON_SYSTEM_STATS_H
#define BLACKBOX_COMMON_SYSTEM_STATS_H

#include <cstddef>
#include <cstdint>

namespace blackbox::common {

    class SystemStats {
    public:
        // Singleton
        SystemStats(const SystemStats&) = delete;
        SystemStats& operator=(const SystemStats&) = delete;
        static SystemStats& instance();

        /**
         * @brief Get Resident Set Size (Physical Memory) usage.
         * @return Bytes used by the process.
         */
        size_t get_memory_usage_bytes();

        /**
         * @brief Calculate CPU usage percentage since last call.
         * 
         * This is stateful. It compares /proc/stat times between calls.
         * 
         * @return double Percentage (0.0 to 100.0 * NumCores)
         */
        double get_cpu_usage_percent();

        /**
         * @brief Get process uptime.
         * @return Seconds since start.
         */
        long get_uptime_seconds();

    private:
        SystemStats();

        // CPU Calculation State
        unsigned long long last_total_time_ = 0;
        unsigned long long last_proc_time_ = 0;
        
        long page_size_kb_ = 4; // Default 4KB
    };

} // namespace blackbox::common

#endif // BLACKBOX_COMMON_SYSTEM_STATS_H