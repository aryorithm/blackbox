/**
 * @file pipeline.cpp
 * @brief Implementation of the Main Orchestrator.
 * 
 * Integrates Ingestion, Logic, AI, Storage, and Ops into a cohesive
 * high-performance loop.
 */

#include "blackbox/core/pipeline.h"
#include "blackbox/common/settings.h"
#include "blackbox/common/logger.h"
#include "blackbox/common/metrics.h"
#include "blackbox/common/thread_utils.h"
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
        
        // 2. Setup UDP Server (Ingestion)
        udp_server_ = std::make_unique<ingest::UdpServer>(
            *io_context_, 
            settings.network().udp_port, 
            ring_buffer_
        );

        // 3. Setup Logic Engines
        try {
            // A. AI Brain (xInfer)
            brain_ = std::make_unique<analysis::InferenceEngine>(settings.ai().model_path);
            
            // B. Rule Engine (Signatures)
            rule_engine_ = std::make_unique<analysis::RuleEngine>();
            // rule_engine_->load_rules("config/rules.yaml"); // Future TODO

            // C. GeoIP Service (Enrichment)
            geoip_ = std::make_unique<enrichment::GeoIPService>("config/GeoLite2-City.mmdb");

            // D. Admin/Ops Server
            admin_server_ = std::make_unique<AdminServer>(8081);

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
    // Lifecycle: Start
    // =========================================================
    void Pipeline::start() {
        if (running_) return;
        running_ = true;

        LOG_INFO("Spawning Worker Threads...");

        // 1. Start Admin Server (Ops)
        admin_server_->start();

        // 2. Start Network Thread (Ingestion)
        ingest_thread_ = std::thread(&Pipeline::ingest_worker, this);

        // 3. Start Processing Thread (Logic)
        processing_thread_ = std::thread(&Pipeline::processing_worker, this);

        LOG_INFO("Pipeline Active. Kinetic Defense Online.");
    }

    // =========================================================
    // Lifecycle: Stop
    // =========================================================
    void Pipeline::stop() {
        if (!running_) return;
        LOG_WARN("Stopping Pipeline...");
        
        running_ = false;

        // Stop Network IO
        if (io_context_) io_context_->stop();

        // Stop Ops
        if (admin_server_) admin_server_->stop();

        // Join Threads
        if (ingest_thread_.joinable()) ingest_thread_.join();
        if (processing_thread_.joinable()) processing_thread_.join();

        LOG_INFO("Pipeline Stopped.");
    }

    bool Pipeline::is_healthy() const {
        return running_;
    }

    // =========================================================
    // Worker 1: Ingestion (Network IO)
    // =========================================================
    void Pipeline::ingest_worker() {
        // 1. Thread Tuning
        common::ThreadUtils::set_current_thread_name("BB_Ingest");
        common::ThreadUtils::pin_current_thread_to_core(0); // Core 0 for Network
        common::ThreadUtils::set_realtime_priority(90);     // Max Priority

        try {
            // Infinite loop handling UDP packets -> RingBuffer
            io_context_->run(); 
        } catch (const std::exception& e) {
            LOG_CRITICAL("Ingestion Thread Crashed: " + std::string(e.what()));
        }
    }

    // =========================================================
    // Worker 2: Processing (The Hot Path)
    // =========================================================
    void Pipeline::processing_worker() {
        // 1. Thread Tuning
        common::ThreadUtils::set_current_thread_name("BB_Brain");
        common::ThreadUtils::pin_current_thread_to_core(1); // Core 1 for Logic/AI
        common::ThreadUtils::set_realtime_priority(80);     // High Priority

        const int BATCH_SIZE = common::Settings::instance().ai().batch_size;
        const float AI_THRESHOLD = common::Settings::instance().ai().anomaly_threshold;

        // Local buffer for Micro-Batching
        std::vector<parser::ParsedLog> batch_logs;
        batch_logs.reserve(BATCH_SIZE);

        ingest::LogEvent raw_event;

        while (running_) {
            
            // ==========================================
            // STEP 1: MICRO-BATCHING
            // ==========================================
            // Drain up to BATCH_SIZE items from the RingBuffer
            int collected = 0;
            while (collected < BATCH_SIZE && ring_buffer_.pop(raw_event)) {
                // Parse immediately (Zero Copy StringView)
                // Note: We copy to ParsedLog struct which owns data for storage safety
                batch_logs.push_back(parser_.process(raw_event));
                collected++;
            }

            // Yield if empty to save CPU electricity/heat
            if (collected == 0) {
                std::this_thread::yield(); 
                continue;
            }

            // ==========================================
            // STEP 2: LOGIC LOOP
            // ==========================================
            
            for (auto& log : batch_logs) {
                float final_score = 0.0f;
                bool is_critical = false;
                std::string alert_reason = "";

                // --- A. Enrichment (GeoIP) ---
                auto loc = geoip_->lookup(log.host); // Assuming host is IP
                if (loc) {
                    log.country = loc->country_iso;
                    log.lat = loc->latitude;
                    log.lon = loc->longitude;
                }

                // --- B. Static Analysis (Rule Engine) ---
                // Fast check: Does this match a known signature?
                auto rule_hit = rule_engine_->evaluate(log);
                
                if (rule_hit) {
                    // HIT: We trust rules 100%. Skip AI to save GPU cycles.
                    final_score = 1.0f;
                    is_critical = true;
                    alert_reason = "Rule: " + *rule_hit;
                } 
                else {
                    // --- C. Dynamic Analysis (AI Engine) ---
                    // MISS: Run the Neural Network
                    final_score = brain_->evaluate(log.embedding_vector);
                    
                    if (final_score > AI_THRESHOLD) {
                        is_critical = true;
                        alert_reason = "AI Anomaly Detection";
                    }
                    
                    common::Metrics::instance().inc_inferences_run(1);
                }

                // --- D. Active Defense (Alert Manager) ---
                if (is_critical) {
                    common::Metrics::instance().inc_threats_detected(1);
                    
                    // Trigger Blocking / Notifications
                    analysis::AlertManager::instance().trigger_alert(
                        log.host, final_score, alert_reason
                    );
                }

                // --- E. Persistence (Storage Engine) ---
                // Send enriched log + score to the Database
                storage_.enqueue(log, final_score);
            }

            // ==========================================
            // STEP 3: CLEANUP
            // ==========================================
            batch_logs.clear(); 
        }
    }

} // namespace blackbox::core