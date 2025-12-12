/**
 * @file pipeline.cpp
 * @brief Implementation of the Orchestrator.
 */

#include "blackbox/core/pipeline.h"
#include "blackbox/common/settings.h"
#include "blackbox/common/logger.h"
#include "blackbox/common/metrics.h"
#include "blackbox/common/thread_utils.h"
#include "blackbox/common/string_utils.h"
#include "blackbox/analysis/alert_manager.h"
#include <iostream>
#include <chrono>

namespace blackbox::core {

    // =========================================================
    // Constructor
    // =========================================================
    Pipeline::Pipeline() {
        LOG_INFO("Initializing Blackbox Pipeline components...");

        const auto& settings = common::Settings::instance();

        // 1. Setup Network Context
        io_context_ = std::make_shared<boost::asio::io_context>();
        
        // 2. Setup UDP Server
        udp_server_ = std::make_unique<ingest::UdpServer>(
            *io_context_, 
            ring_buffer_ // UdpServer reads port from Settings internally or passed via ctor
        );

        // 3. Setup Logic Engines
        try {
            // A. AI Brain (xInfer)
            brain_ = std::make_unique<analysis::InferenceEngine>(settings.ai().model_path);
            
            // B. Rule Engine (Signatures)
            rule_engine_ = std::make_unique<analysis::RuleEngine>();
            rule_engine_->load_rules(settings.enrichment().rules_config_path);

            // C. GeoIP Service (Enrichment)
            geoip_ = std::make_unique<enrichment::GeoIPService>(settings.enrichment().geoip_db_path);

            // D. Admin Server (Prometheus/Health)
            admin_server_ = std::make_unique<AdminServer>(settings.network().admin_port);

            // E. Redis Client (Real-time Alerts)
            redis_ = std::make_unique<storage::RedisClient>(
                settings.db().redis_host,
                settings.db().redis_port
            );

        } catch (const std::exception& e) {
            LOG_CRITICAL("Failed to initialize pipeline components: " + std::string(e.what()));
            throw; // Fatal error, crash the app
        }
    }

    // =========================================================
    // Destructor
    // =========================================================
    Pipeline::~Pipeline() {
        stop();
    }

    // =========================================================
    // Start
    // =========================================================
    void Pipeline::start() {
        if (running_) return;
        running_ = true;

        LOG_INFO("Spawning Worker Threads...");

        // 1. Start Ops Server
        admin_server_->start();

        // 2. Start Network Thread
        ingest_thread_ = std::thread(&Pipeline::ingest_worker, this);

        // 3. Start Processing Thread
        processing_thread_ = std::thread(&Pipeline::processing_worker, this);

        LOG_INFO("Pipeline Active. Kinetic Defense Online.");
    }

    // =========================================================
    // Stop
    // =========================================================
    void Pipeline::stop() {
        if (!running_) return;
        LOG_WARN("Stopping Pipeline...");
        
        running_ = false;

        if (io_context_) io_context_->stop();
        if (admin_server_) admin_server_->stop();

        if (ingest_thread_.joinable()) ingest_thread_.join();
        if (processing_thread_.joinable()) processing_thread_.join();

        LOG_INFO("Pipeline Stopped.");
    }

    bool Pipeline::is_healthy() const {
        return running_;
    }

    // =========================================================
    // Ingestion Worker (Core 0)
    // =========================================================
    void Pipeline::ingest_worker() {
        common::ThreadUtils::set_current_thread_name("BB_Ingest");
        common::ThreadUtils::pin_current_thread_to_core(0);
        common::ThreadUtils::set_realtime_priority(90);

        try {
            io_context_->run();
        } catch (const std::exception& e) {
            LOG_CRITICAL("Ingestion Thread Crashed: " + std::string(e.what()));
        }
    }

    // =========================================================
    // Processing Worker (Core 1)
    // =========================================================
    void Pipeline::processing_worker() {
        common::ThreadUtils::set_current_thread_name("BB_Brain");
        common::ThreadUtils::pin_current_thread_to_core(1);
        common::ThreadUtils::set_realtime_priority(80);

        const auto& settings = common::Settings::instance();
        const int BATCH_SIZE = settings.ai().batch_size;
        const float AI_THRESHOLD = settings.ai().anomaly_threshold;

        // Micro-batch buffer
        std::vector<parser::ParsedLog> batch_logs;
        batch_logs.reserve(BATCH_SIZE);

        ingest::LogEvent raw_event;

        while (running_) {
            
            // -------------------------------------------------
            // 1. Fetch Batch from RingBuffer
            // -------------------------------------------------
            int collected = 0;
            while (collected < BATCH_SIZE && ring_buffer_.pop(raw_event)) {
                // Parse (Zero Copy)
                batch_logs.push_back(parser_.process(raw_event));
                collected++;
            }

            if (collected == 0) {
                std::this_thread::yield();
                continue;
            }

            // -------------------------------------------------
            // 2. Process Logic
            // -------------------------------------------------
            for (auto& log : batch_logs) {
                float final_score = 0.0f;
                bool is_critical = false;
                std::string alert_reason = "";

                // A. GeoIP Enrichment
                // We use std::string(log.host) because lookup expects null-terminated string/view
                auto loc = geoip_->lookup(log.host);
                if (loc) {
                    log.country = loc->country_iso;
                    log.lat = loc->latitude;
                    log.lon = loc->longitude;
                }

                // B. Rule Engine (Static)
                auto rule_hit = rule_engine_->evaluate(log);
                if (rule_hit) {
                    final_score = 1.0f;
                    is_critical = true;
                    alert_reason = "Rule: " + *rule_hit;
                } 
                else {
                    // C. AI Engine (Dynamic)
                    final_score = brain_->evaluate(log.embedding_vector);
                    if (final_score > AI_THRESHOLD) {
                        is_critical = true;
                        alert_reason = "AI Anomaly Detection";
                    }
                    common::Metrics::instance().inc_inferences_run(1);
                }

                // D. Action (If Critical)
                if (is_critical) {
                    common::Metrics::instance().inc_threats_detected(1);
                    
                    // 1. Active Defense (Block IP)
                    analysis::AlertManager::instance().trigger_alert(
                        log.host, final_score, alert_reason
                    );

                    // 2. Notify Dashboard (Redis Pub/Sub)
                    // Construct JSON manually for speed
                    std::string json = "{";
                    json += "\"ts\": " + std::to_string(log.timestamp) + ",";
                    json += "\"ip\": \"" + std::string(log.host) + "\",";
                    json += "\"score\": " + std::to_string(final_score) + ",";
                    json += "\"reason\": \"" + alert_reason + "\",";
                    json += "\"country\": \"" + log.country + "\",";
                    json += "\"msg\": \"" + common::StringUtils::escape_sql(std::string(log.message)) + "\"";
                    json += "}";

                    redis_->publish(settings.db().redis_channel, json);
                }

                // E. Persistence (ClickHouse)
                storage_.enqueue(log, final_score);
            }

            // -------------------------------------------------
            // 3. Reset
            // -------------------------------------------------
            batch_logs.clear();
        }
    }

} // namespace blackbox::core