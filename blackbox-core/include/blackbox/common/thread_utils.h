/**
 * @file thread_utils.h
 * @brief Low-latency System Optimizations.
 * 
 * Provides utilities to pin threads to specific CPU cores 
 * and set real-time scheduling priorities.
 */

#ifndef BLACKBOX_COMMON_THREAD_UTILS_H
#define BLACKBOX_COMMON_THREAD_UTILS_H

#include <string>
#include <thread>
#include <vector>

namespace blackbox::common {

    class ThreadUtils {
    public:
        /**
         * @brief Sets the name of the current thread.
         * Visible in tools like 'htop' and 'gdb'.
         * Limit: 15 chars on Linux.
         */
        static void set_current_thread_name(const std::string& name);

        /**
         * @brief Pins the current thread to a specific CPU Core ID.
         * 
         * @param core_id The CPU index (0, 1, 2...)
         * @return true if successful
         */
        static bool pin_current_thread_to_core(int core_id);

        /**
         * @brief Sets the current thread to Real-Time Priority (SCHED_FIFO).
         * WARNING: Requires root privileges (CAP_SYS_NICE).
         * 
         * @param priority Priority level (1-99, higher is more urgent)
         * @return true if successful
         */
        static bool set_realtime_priority(int priority);

        /**
         * @brief Returns the number of available hardware concurrency.
         */
        static unsigned int get_num_cores();
    };

} // namespace blackbox::common

#endif // BLACKBOX_COMMON_THREAD_UTILS_H