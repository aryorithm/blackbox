/**
 * @file settings.cpp
 * @brief Implementation of Configuration Loading.
 */

#include "blackbox/common/settings.h"
#include <cstdlib> // for std::getenv
#include <iostream>
#include <string>

namespace blackbox::common {

    // =========================================================
    // Singleton Instance
    // =========================================================
    Settings& Settings::instance() {
        static Settings instance;
        return instance;
    }

    // =========================================================
    // Helper: Parse Env Var safely
    // =========================================================
    static std::string get_env_string(const char* key, const std::string& default_val) {
        const char* val = std::getenv(key);
        return val ? std::string(val) : default_val;
    }

    static int get_env_int(const char* key, int default_val) {
        const char* val = std::getenv(key);
        if (val) {
            try {
                return std::stoi(val);
            } catch (...) {
                std::cerr << "[WARN] Invalid integer for ENV " << key << ", using default." << std::endl;
            }
        }
        return default_val;
    }

    static float get_env_float(const char* key, float default_val) {
        const char* val = std::getenv(key);
        if (val) {
            try {
                return std::stof(val);
            } catch (...) {
                std::cerr << "[WARN] Invalid float for ENV " << key << ", using default." << std::endl;
            }
        }
        return default_val;
    }

    // =========================================================
    // Load Logic
    // =========================================================
    void Settings::load_from_env() {
        std::cout << "[INIT] Loading Configuration..." << std::endl;

        // Network
        network_.udp_port = static_cast<uint16_t>(get_env_int("BLACKBOX_UDP_PORT", 514));
        network_.ring_buffer_size = get_env_int("BLACKBOX_RING_BUFFER_SIZE", 65536);

        // AI
        ai_.model_path = get_env_string("BLACKBOX_MODEL_PATH", "/app/models/autoencoder.plan");
        ai_.anomaly_threshold = get_env_float("BLACKBOX_ANOMALY_THRESHOLD", 0.8f);
        ai_.batch_size = get_env_int("BLACKBOX_AI_BATCH_SIZE", 32);

        // Database
        db_.clickhouse_url = get_env_string("BLACKBOX_CLICKHOUSE_URL", "http://clickhouse:8123");
        db_.flush_batch_size = get_env_int("BLACKBOX_DB_BATCH_SIZE", 1000);
        db_.flush_interval_ms = get_env_int("BLACKBOX_DB_FLUSH_MS", 1000);

        std::cout << "[INIT] Config Loaded. Listening on Port: " << network_.udp_port << std::endl;
        std::cout << "[INIT] DB Target: " << db_.clickhouse_url << std::endl;
    }

} // namespace blackbox::common