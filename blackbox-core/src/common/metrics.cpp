/**
 * @file metrics.cpp
 * @brief Implementation of Application Observability.
 */

#include "blackbox/common/metrics.h"
#include "blackbox/common/logger.h"
#include "blackbox/common/system_stats.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace blackbox::common {

    // =========================================================
    // Singleton Instance
    // =========================================================
    Metrics& Metrics::instance() {
        static Metrics instance;
        return instance;
    }

    // =========================================================
    // Destructor
    // =========================================================
    Metrics::~Metrics() {
        stop();
    }

    // =========================================================
    // Atomic Increments (The Hot Path)
    // =========================================================
    // Uses memory_order_relaxed because we don't need these to act 
    // as fences for other data, we just want the count to be accurate.

    void Metrics::inc_packets_received(size_t count) {
        packets_rx_.fetch_add(count, std::memory_order_relaxed);
    }

    void Metrics::inc_packets_dropped(size_t count) {
        packets_dropped_.fetch_add(count, std::memory_order_relaxed);
    }

    void Metrics::inc_inferences_run(size_t count) {
        inferences_.fetch_add(count, std::memory_order_relaxed);
    }

    void Metrics::inc_threats_detected(size_t count) {
        threats_.fetch_add(count, std::memory_order_relaxed);
    }

    void Metrics::inc_db_rows_written(size_t count) {
        db_written_.fetch_add(count, std::memory_order_relaxed);
    }

    void Metrics::inc_db_errors(size_t count) {
        db_errors_.fetch_add(count, std::memory_order_relaxed);
    }

    // =========================================================
    // Lifecycle Management
    // =========================================================

    void Metrics::start_reporter(int interval_seconds) {
        if (running_) return;
        running_ = true;
        reporter_thread_ = std::thread(&Metrics::reporter_worker, this, interval_seconds);
        LOG_INFO("Metrics Reporter started. Interval: " + std::to_string(interval_seconds) + "s");
    }

    void Metrics::stop() {
        if (!running_) return;
        running_ = false;
        if (reporter_thread_.joinable()) {
            reporter_thread_.join();
        }
    }

    // =========================================================
    // Background Reporter (Console Heartbeat)
    // =========================================================
    void Metrics::reporter_worker(int interval_seconds) {
        
        uint64_t last_rx = 0;
        
        while (running_) {
            // Sleep for interval
            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
            if (!running_) break;

            // 1. Snapshot values (Atomic Load)
            uint64_t rx = packets_rx_.load(std::memory_order_relaxed);
            uint64_t drop = packets_dropped_.load(std::memory_order_relaxed);
            uint64_t infer = inferences_.load(std::memory_order_relaxed);
            uint64_t alerts = threats_.load(std::memory_order_relaxed);
            uint64_t db = db_written_.load(std::memory_order_relaxed);

            // 2. Calculate EPS (Events Per Second) based on delta
            uint64_t delta = rx - last_rx;
            double eps = (double)delta / interval_seconds;
            last_rx = rx;

            // 3. Get System Stats
            double cpu_usage = SystemStats::instance().get_cpu_usage_percent();
            size_t ram_usage = SystemStats::instance().get_memory_usage_bytes() / 1024 / 1024; // MB

            // 4. Format Log
            std::stringstream ss;
            ss << "STATS [" << interval_seconds << "s] | "
               << "EPS: " << std::fixed << std::setprecision(1) << eps << " | "
               << "Total RX: " << rx << " | "
               << "Drops: " << drop << " | "
               << "Threats: " << alerts << " | "
               << "DB: " << db << " | "
               << "CPU: " << std::fixed << std::setprecision(1) << cpu_usage << "% | "
               << "RAM: " << ram_usage << "MB";

            LOG_INFO(ss.str());
        }
    }

    // =========================================================
    // Prometheus Exporter (For Admin Server)
    // =========================================================
    std::string Metrics::get_prometheus_metrics() {
        std::stringstream ss;
        
        // Snapshot atomic values
        uint64_t rx = packets_rx_.load(std::memory_order_relaxed);
        uint64_t drops = packets_dropped_.load(std::memory_order_relaxed);
        uint64_t inf = inferences_.load(std::memory_order_relaxed);
        uint64_t thr = threats_.load(std::memory_order_relaxed);
        uint64_t db = db_written_.load(std::memory_order_relaxed);
        uint64_t err = db_errors_.load(std::memory_order_relaxed);

        // Snapshot System Stats
        double cpu = SystemStats::instance().get_cpu_usage_percent();
        size_t ram = SystemStats::instance().get_memory_usage_bytes();

        // ---------------------------------------------------------
        // Format: Prometheus Text Protocol
        // ---------------------------------------------------------

        // App Metrics
        ss << "# HELP blackbox_packets_total Total UDP packets received\n"
           << "# TYPE blackbox_packets_total counter\n"
           << "blackbox_packets_total " << rx << "\n\n";

        ss << "# HELP blackbox_packets_dropped_total Total packets dropped (buffer full/ratelimit)\n"
           << "# TYPE blackbox_packets_dropped_total counter\n"
           << "blackbox_packets_dropped_total " << drops << "\n\n";

        ss << "# HELP blackbox_inferences_total Total AI inferences run\n"
           << "# TYPE blackbox_inferences_total counter\n"
           << "blackbox_inferences_total " << inf << "\n\n";

        ss << "# HELP blackbox_threats_detected_total Total critical threats found\n"
           << "# TYPE blackbox_threats_detected_total counter\n"
           << "blackbox_threats_detected_total " << thr << "\n\n";

        ss << "# HELP blackbox_db_written_total Total rows flushed to ClickHouse\n"
           << "# TYPE blackbox_db_written_total counter\n"
           << "blackbox_db_written_total " << db << "\n\n";

        ss << "# HELP blackbox_db_errors_total Total DB write failures\n"
           << "# TYPE blackbox_db_errors_total counter\n"
           << "blackbox_db_errors_total " << err << "\n\n";

        // System Metrics
        ss << "# HELP blackbox_process_cpu_percent CPU usage percentage (normalized)\n"
           << "# TYPE blackbox_process_cpu_percent gauge\n"
           << "blackbox_process_cpu_percent " << cpu << "\n\n";

        ss << "# HELP blackbox_process_memory_bytes Resident memory size in bytes\n"
           << "# TYPE blackbox_process_memory_bytes gauge\n"
           << "blackbox_process_memory_bytes " << ram << "\n\n";

        return ss.str();
    }

} // namespace blackbox::common