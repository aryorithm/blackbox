/**
 * @file pipeline.h
 * @brief The Main Data Processing Engine.
 * 
 * Orchestrates the flow:
 * Ingest (UDP) -> RingBuffer -> Parse -> Batch -> AI Inference -> Alert -> Storage
 */

#ifndef BLACKBOX_CORE_PIPELINE_H
#define BLACKBOX_CORE_PIPELINE_H

#include <vector>
#include <thread>
#include <atomic>
#include <memory>

// Include component headers
#include "blackbox/ingest/ring_buffer.h"
#include "blackbox/ingest/udp_server.h"
#include "blackbox/parser/parser_engine.h"
#include "blackbox/analysis/inference_engine.h"
#include "blackbox/storage/storage_engine.h"

namespace blackbox::core {

    class Pipeline {
    public:
        Pipeline();
        ~Pipeline();

        /**
         * @brief Boots up the system.
         * 1. Loads Settings
         * 2. Inits AI & DB connections
         * 3. Spawns Threads
         */
        void start();

        /**
         * @brief Graceful shutdown.
         * Flushes buffers and joins threads.
         */
        void stop();

        /**
         * @brief Checks if pipeline is healthy.
         */
        bool is_healthy() const;

    private:
        /**
         * @brief The Network Thread Function.
         * Runs the Boost.Asio event loop.
         */
        void ingest_worker();

        /**
         * @brief The AI/Logic Thread Function.
         * Pops from RingBuffer, Batches, Runs Inference, Routes data.
         */
        void processing_worker();

        // STATE
        std::atomic<bool> running_{false};
        
        // THREADS
        std::thread ingest_thread_;
        std::thread processing_thread_;

        // COMPONENTS
        // 1. The Buffer (Shared Memory)
        ingest::RingBuffer<65536> ring_buffer_;

        // 2. The Network Layer
        // Uses unique_ptr because it needs io_context which is non-copyable
        std::shared_ptr<boost::asio::io_context> io_context_;
        std::unique_ptr<ingest::UdpServer> udp_server_;

        // 3. The Logic Layer
        parser::ParserEngine parser_;
        std::unique_ptr<analysis::InferenceEngine> brain_;
        
        // 4. The Persistence Layer
        storage::StorageEngine storage_;
    };

} // namespace blackbox::core

#endif // BLACKBOX_CORE_PIPELINE_H