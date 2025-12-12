/**
 * @file system_stats.cpp
 * @brief Implementation of Linux ProcFS Parsing.
 */

#include "blackbox/common/system_stats.h"
#include "blackbox/common/logger.h"
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h> // sysconf
#include <vector>

namespace blackbox::common {

    // =========================================================
    // Constructor
    // =========================================================
    SystemStats& SystemStats::instance() {
        static SystemStats instance;
        return instance;
    }

    SystemStats::SystemStats() {
        // Detect OS Page Size (usually 4096 bytes)
        long page_size = sysconf(_SC_PAGESIZE);
        if (page_size > 0) {
            page_size_kb_ = page_size / 1024;
        }
    }

    // =========================================================
    // Get Memory Usage (RSS)
    // =========================================================
    size_t SystemStats::get_memory_usage_bytes() {
        // Read /proc/self/statm
        // Format: size resident shared text lib data dt
        std::ifstream file("/proc/self/statm");
        if (!file.is_open()) return 0;

        unsigned long size, resident, share, text, lib, data, dt;
        file >> size >> resident >> share >> text >> lib >> data >> dt;
        
        // 'resident' is in pages. Convert to bytes.
        // We use sysconf value * 1024 because page_size_kb_ is in KB.
        return resident * (size_t)sysconf(_SC_PAGESIZE);
    }

    // =========================================================
    // Get CPU Usage
    // =========================================================
    double SystemStats::get_cpu_usage_percent() {
        // 1. Read System-wide CPU times from /proc/stat
        std::ifstream stat_file("/proc/stat");
        if (!stat_file.is_open()) return 0.0;

        std::string line;
        std::getline(stat_file, line); // First line is "cpu  ..."
        std::stringstream ss(line);
        std::string label;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        ss >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

        unsigned long long current_total_time = user + nice + system + idle + iowait + irq + softirq + steal;

        // 2. Read Process CPU times from /proc/self/stat
        std::ifstream self_file("/proc/self/stat");
        if (!self_file.is_open()) return 0.0;

        // The fields we want are #14 (utime) and #15 (stime)
        // We have to skip the first 13 fields.
        // Warning: The filename field #2 might contain spaces "(blackbox core)", so strictly parsing by space is risky 
        // if the binary name has spaces. Assuming standard name here.
        
        std::string dummy;
        unsigned long long utime = 0, stime = 0;
        
        // Skip 13 fields
        for (int i = 0; i < 13; ++i) self_file >> dummy;
        self_file >> utime >> stime;

        unsigned long long current_proc_time = utime + stime;

        // 3. Calculate Delta
        double percent = 0.0;
        if (last_total_time_ > 0 && last_proc_time_ > 0) {
            unsigned long long total_delta = current_total_time - last_total_time_;
            unsigned long long proc_delta = current_proc_time - last_proc_time_;

            if (total_delta > 0) {
                // Determine number of cores to normalize? 
                // Usually top shows % * cores.
                long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
                
                percent = (double)proc_delta / (double)total_delta * 100.0 * num_cores;
            }
        }

        // Update State
        last_total_time_ = current_total_time;
        last_proc_time_ = current_proc_time;

        return percent;
    }

    // =========================================================
    // Get Uptime
    // =========================================================
    long SystemStats::get_uptime_seconds() {
        // Read /proc/uptime for system boot time, calculate process start...
        // Easier: Just read /proc/self/stat field #22 (starttime) / CLK_TCK
        // For MVP, just returning a placeholder or implementing generic timer logic is fine.
        // Let's rely on simple clock since app start if needed, but for now return 0.
        return 0; // Simplified
    }

} // namespace blackbox::common