/**
 * @file alert_manager.h
 * @brief Active Defense & Incident Response System.
 * 
 * Handles immediate reactions to Critical Threats.
 * Includes logic to prevent alert spam (Deduplication).
 */

#ifndef BLACKBOX_ANALYSIS_ALERT_MANAGER_H
#define BLACKBOX_ANALYSIS_ALERT_MANAGER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace blackbox::analysis {

    class AlertManager {
    public:
        // Singleton pattern to maintain global deduplication state
        AlertManager(const AlertManager&) = delete;
        AlertManager& operator=(const AlertManager&) = delete;

        static AlertManager& instance();

        /**
         * @brief Evaluate a threat and take action if necessary.
         * 
         * @param source_ip The attacker's IP
         * @param score The anomaly score (0.0 - 1.0)
         * @param message The raw log message
         */
        void trigger_alert(std::string_view source_ip, float score, std::string_view message);

    private:
        AlertManager() = default;

        /**
         * @brief Execute a system command to block the IP.
         * WARNING: Only runs if configuration allows active blocking.
         */
        void execute_block_action(const std::string& ip);

        /**
         * @brief Checks if we have already alerted on this IP recently.
         * @return true if we should alert, false if we are in cooldown.
         */
        bool should_trigger(const std::string& ip);

        // STATE
        // Map of IP Address -> Last Alert Timestamp
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> cooldown_map_;
        std::mutex map_mutex_;

        // CONFIG
        const int COOLDOWN_SECONDS = 300; // 5 Minutes
        const float CRITICAL_THRESHOLD = 0.95f;
    };

} // namespace blackbox::analysis

#endif // BLACKBOX_ANALYSIS_ALERT_MANAGER_H