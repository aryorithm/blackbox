/**
 * @file settings.h
 * @brief Global Configuration Manager.
 */

#ifndef BLACKBOX_COMMON_SETTINGS_H
#define BLACKBOX_COMMON_SETTINGS_H

#include <string>
#include <cstdint>

namespace blackbox::common {

    struct NetworkConfig {
        uint16_t udp_port = 514;
        uint16_t admin_port = 8081;
        size_t ring_buffer_size = 65536;
    };

    struct AIConfig {
        std::string model_path = "models/autoencoder.plan";
        std::string vocab_path = "config/vocab.txt";
        std::string scaler_path = "config/scaler_params.txt";
        float anomaly_threshold = 0.8f;
        int batch_size = 32;
    };

    struct EnrichmentConfig {
        std::string geoip_db_path = "config/GeoLite2-City.mmdb";
        std::string rules_config_path = "config/rules.yaml";
    };

    struct DatabaseConfig {
        // ClickHouse (Logs)
        std::string clickhouse_url = "http://localhost:8123";
        size_t flush_batch_size = 1000;
        int flush_interval_ms = 1000;

        // Redis (Real-time Alerts)
        std::string redis_host = "localhost";
        int redis_port = 6379;
        std::string redis_channel = "sentry_alerts";
    };

    class Settings {
    public:
        Settings(const Settings&) = delete;
        Settings& operator=(const Settings&) = delete;

        static Settings& instance();

        void load_from_env();

        const NetworkConfig& network() const { return network_; }
        const AIConfig& ai() const { return ai_; }
        const EnrichmentConfig& enrichment() const { return enrichment_; }
        const DatabaseConfig& db() const { return db_; }

    private:
        Settings() = default;

        NetworkConfig network_;
        AIConfig ai_;
        EnrichmentConfig enrichment_;
        DatabaseConfig db_;
    };

} // namespace blackbox::common

#endif // BLACKBOX_COMMON_SETTINGS_H