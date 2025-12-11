/**
 * @file signal_handler.cpp
 * @brief Implementation of OS Signal Interception.
 */

#include "blackbox/common/signal_handler.h"
#include "blackbox/common/logger.h"
#include <csignal>
#include <iostream>

namespace blackbox::common {

    // =========================================================
    // Singleton Instance
    // =========================================================
    SignalHandler& SignalHandler::instance() {
        static SignalHandler instance;
        return instance;
    }

    // =========================================================
    // Static Handler Callback
    // =========================================================
    // This function is called by the OS. It must be static.
    void SignalHandler::handle_signal(int signal) {
        if (signal == SIGINT) {
            std::cout << "\n[SYS] SIGINT received (Ctrl+C). Shutting down..." << std::endl;
        } else if (signal == SIGTERM) {
            std::cout << "\n[SYS] SIGTERM received (Docker Stop). Shutting down..." << std::endl;
        }
        
        // Notify the instance
        instance().trigger_shutdown();
    }

    // =========================================================
    // Register Handlers
    // =========================================================
    void SignalHandler::register_handlers() {
        std::signal(SIGINT, handle_signal);  // Handle Ctrl+C
        std::signal(SIGTERM, handle_signal); // Handle `docker stop`
        LOG_INFO("Signal Handlers registered. Waiting for signals...");
    }

    // =========================================================
    // Logic
    // =========================================================
    bool SignalHandler::is_running() const {
        // memory_order_relaxed is fine for simple boolean flags
        return running_.load(std::memory_order_relaxed);
    }

    void SignalHandler::trigger_shutdown() {
        bool expected = true;
        // Compare Exchange prevents multiple triggers
        if (running_.compare_exchange_strong(expected, false)) {
            // Wake up the main thread if it is waiting
            cv_.notify_all();
        }
    }

    void SignalHandler::wait_for_signal() {
        std::unique_lock<std::mutex> lock(mutex_);
        // Wait until running_ becomes false
        cv_.wait(lock, [this] { return !running_; });
        LOG_INFO("Main thread unblocked. Proceeding to cleanup.");
    }

} // namespace blackbox::common