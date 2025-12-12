/**
 * @file rate_limiter.h
 * @brief DoS Protection for the Ingestion Layer.
 * 
 * Implements the Token Bucket algorithm to limit logs per second
 * from specific source IPs. Prevents "Noisy Neighbor" problems.
 */

#ifndef BLACKBOX_INGEST_RATE_LIMITER_H
#define BLACKBOX_INGEST_RATE_LIMITER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace blackbox::ingest {

    struct TokenBucket {
        double tokens;           // Current available tokens
        double max_burst;        // Capacity of the bucket
        double refill_rate;      // Tokens added per second
        std::chrono::steady_clock::time_point last_refill;
    };

    class RateLimiter {
    public:
        // Singleton Access
        RateLimiter(const RateLimiter&) = delete;
        RateLimiter& operator=(const RateLimiter&) = delete;
        static RateLimiter& instance();

        /**
         * @brief Checks if an IP is allowed to send a log.
         * 
         * @param ip_address The source IP
         * @return true if allowed, false if limit exceeded (Drop packet)
         */
        bool should_allow(std::string_view ip_address);

        /**
         * @brief Periodic cleanup of old IP entries to prevent memory leaks.
         * Should be called occasionally by a background thread.
         */
        void cleanup();

    private:
        RateLimiter();

        // Default Limits
        const double DEFAULT_RATE = 100.0;   // 100 logs/sec per IP
        const double DEFAULT_BURST = 500.0;  // Allow burst of 500

        std::mutex mutex_;
        std::unordered_map<std::string, TokenBucket> buckets_;
    };

} // namespace blackbox::ingest

#endif // BLACKBOX_INGEST_RATE_LIMITER_H