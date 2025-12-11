/**
 * @file settings.h
 * @brief Global Configuration Manager.
 * 
 * Centralizes all runtime settings (Ports, Paths, URLs).
 * Supports loading from Environment Variables for Docker/K8s compatibility.
 */

#ifndef BLACKBOX_COMMON_SETTINGS_H
#define BLACKBOX_COMMON_SETTINGS_H

#include <string>
#include <cstdint>

namespace blackbox::common {

    // Grouping settings by subsystem
    struct NetworkConfig {
        uint16_t udp_port = 514;
        size_t ring_buffer_size = 65536;
    };

    struct AIConfig {
        std::string model_path = "models/autoencoder.plan";
        float anomaly_threshold = 0.8f;
        int batch_size = 32;
    };

    struct DatabaseConfig {
        std::string clickhouse_url = "http://localhost:8123";
        size_t flush_batch_size = 1000;
        int flush_interval_ms = 1000;
    };

    class Settings {
    public:
        // Delete copy constructors (Singleton)
        Settings(const Settings&) = delete;
        Settings& operator=(const Settings&) = delete;

        /**
         * @brief Get the global instance.
         */
        static Settings& instance();

        /**
         * @brief Load settings from Environment Variables.
         * 
         * Looks for:
         * - BLACKBOX_UDP_PORT
         * - BLACKBOX_MODEL_PATH
         * - BLACKBOX_CLICKHOUSE_URL
         * - etc.
         */
        void load_from_env();

        // Accessors
        const NetworkConfig& network() const { return network_; }
        const AIConfig& ai() const { return ai_; }
        const DatabaseConfig& db() const { return db_; }

    private:
        Settings() = default; // Private constructor

        NetworkConfig network_;
        AIConfig ai_;
        DatabaseConfig db_;
    };

} // namespace blackbox::common

#endif // BLACKBOX_COMMON_SETTINGS_H