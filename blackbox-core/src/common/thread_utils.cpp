/**
 * @file thread_utils.cpp
 * @brief Implementation of POSIX Thread Management.
 */

#include "blackbox/common/thread_utils.h"
#include "blackbox/common/logger.h"
#include <pthread.h>
#include <sched.h>
#include <cstring> // for strerror
#include <iostream>

namespace blackbox::common {

    // =========================================================
    // Set Name
    // =========================================================
    void ThreadUtils::set_current_thread_name(const std::string& name) {
        // Linux limit is 16 bytes including null terminator
        std::string short_name = name.substr(0, 15);
        
        int rc = pthread_setname_np(pthread_self(), short_name.c_str());
        if (rc != 0) {
            LOG_WARN("Failed to set thread name: " + name);
        }
    }

    // =========================================================
    // CPU Affinity (Pinning)
    // =========================================================
    bool ThreadUtils::pin_current_thread_to_core(int core_id) {
        int num_cores = std::thread::hardware_concurrency();
        if (core_id < 0 || core_id >= num_cores) {
            LOG_ERROR("Invalid Core ID: " + std::to_string(core_id));
            return false;
        }

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);

        int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            LOG_ERROR("Failed to pin thread to Core " + std::to_string(core_id) + 
                      ": " + std::strerror(rc));
            return false;
        }

        LOG_INFO("Thread pinned to CPU Core: " + std::to_string(core_id));
        return true;
    }

    // =========================================================
    // Real-Time Priority
    // =========================================================
    bool ThreadUtils::set_realtime_priority(int priority) {
        // SCHED_FIFO: First In, First Out real-time policy.
        // A thread with this policy will preempt any normal thread.
        int policy = SCHED_FIFO;
        struct sched_param param;
        param.sched_priority = priority;

        int rc = pthread_setschedparam(pthread_self(), policy, &param);
        if (rc != 0) {
            // This usually fails if not running as root/Docker privileged
            LOG_WARN("Failed to set Real-Time Priority (requires CAP_SYS_NICE): " + 
                     std::string(std::strerror(rc)));
            return false;
        }

        LOG_INFO("Thread Priority set to REALTIME (FIFO) Level: " + std::to_string(priority));
        return true;
    }

    unsigned int ThreadUtils::get_num_cores() {
        return std::thread::hardware_concurrency();
    }

} // namespace blackbox::common