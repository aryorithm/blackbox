/**
 * @file block_list_manager.cpp
 * @brief Implementation of Firewall Lifecycle.
 */

#include "blackbox/analysis/block_list_manager.h"
#include "blackbox/common/logger.h"
#include "blackbox/common/settings.h"
#include <cstdlib> // std::system
#include <vector>

namespace blackbox::analysis {

    // =========================================================
    // Singleton
    // =========================================================
    BlockListManager& BlockListManager::instance() {
        static BlockListManager instance;
        return instance;
    }

    // =========================================================
    // Constructor / Destructor
    // =========================================================
    BlockListManager::BlockListManager() : running_(true) {
        // Start the janitor thread
        worker_thread_ = std::thread(&BlockListManager::expiration_worker, this);
        LOG_INFO("Active Defense Manager started. Default ban time: 10m.");
    }

    BlockListManager::~BlockListManager() {
        running_ = false;
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

        // Optional: Clear all bans on shutdown?
        // Usually safer to leave them or persist them to disk.
        // For MVP, we leave them in iptables.
    }

    // =========================================================
    // Block IP
    // =========================================================
    void BlockListManager::block_ip(const std::string& ip, int duration_seconds) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if already blocked
        if (active_blocks_.find(ip) != active_blocks_.end()) {
            // Already blocked. Extend duration?
            // For now, just ignore.
            return;
        }

        // 1. Update State
        BlockEntry entry;
        entry.ip = ip;
        entry.duration_seconds = duration_seconds;
        entry.start_time = std::chrono::steady_clock::now();

        active_blocks_[ip] = entry;

        // 2. Execute System Call
        execute_firewall_command(ip, true);
    }

    // =========================================================
    // Unblock IP
    // =========================================================
    void BlockListManager::unblock_ip(const std::string& ip) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = active_blocks_.find(ip);
        if (it == active_blocks_.end()) {
            return; // Not found
        }

        // 1. Remove from State
        active_blocks_.erase(it);

        // 2. Execute System Call
        execute_firewall_command(ip, false);
    }

    // =========================================================
    // Check Status
    // =========================================================
    bool BlockListManager::is_blocked(const std::string& ip) {
        std::lock_guard<std::mutex> lock(mutex_);
        return active_blocks_.find(ip) != active_blocks_.end();
    }

    // =========================================================
    // Expiration Worker (Background Thread)
    // =========================================================
    void BlockListManager::expiration_worker() {
        while (running_) {
            // Check every 5 seconds
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!running_) break;

            std::vector<std::string> expired_ips;
            auto now = std::chrono::steady_clock::now();

            // 1. Find Expired
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (const auto& [ip, entry] : active_blocks_) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - entry.start_time).count();

                    if (elapsed >= entry.duration_seconds) {
                        expired_ips.push_back(ip);
                    }
                }
            }

            // 2. Remove (Unblock)
            // note: we call unblock_ip which re-locks the mutex.
            // This is safe because we released the lock above.
            for (const auto& ip : expired_ips) {
                LOG_INFO("Ban expired for IP: " + ip + ". Unblocking.");
                unblock_ip(ip);
            }
        }
    }

    // =========================================================
    // System Call Helper
    // =========================================================
    void BlockListManager::execute_firewall_command(const std::string& ip, bool add) {
        // Simple Input Validation to prevent Command Injection
        // (Very basic check: assume valid IP has no spaces or semicolons)
        if (ip.find(' ') != std::string::npos || ip.find(';') != std::string::npos) {
            LOG_ERROR("Invalid IP format in firewall request: " + ip);
            return;
        }

        std::string cmd;
        if (add) {
            // Append rule to DROP packets from this IP
            cmd = "iptables -A INPUT -s " + ip + " -j DROP";
            LOG_CRITICAL("Adding Firewall Rule: " + cmd);
        } else {
            // Delete the specific rule
            cmd = "iptables -D INPUT -s " + ip + " -j DROP";
            LOG_INFO("Removing Firewall Rule: " + cmd);
        }

        // Use std::system.
        // In a container, ensure we are root or have NET_ADMIN capability.
        int ret = std::system(cmd.c_str());

        if (ret != 0) {
            // This might happen if rule doesn't exist during unblock (harmless)
            // or if we lack permissions (critical).
            LOG_WARN("Firewall command returned non-zero exit code.");
        }
    }

} // namespace blackbox::analysis