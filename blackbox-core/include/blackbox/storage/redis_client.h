/**
 * @file redis_client.h
 * @brief Redis Pub/Sub Publisher.
 *
 * Used to broadcast "Critical Alerts" to the Go API (Tower)
 * for real-time dashboard visualization.
 */

#ifndef BLACKBOX_STORAGE_REDIS_CLIENT_H
#define BLACKBOX_STORAGE_REDIS_CLIENT_H

#include <string>
#include <mutex>
// Forward declaration of hiredis context to keep header clean
struct redisContext;

namespace blackbox::storage {

    class RedisClient {
    public:
        /**
         * @brief Initialize connection to Redis.
         *
         * @param host Redis Hostname (e.g., "localhost" or "redis")
         * @param port Redis Port (default 6379)
         */
        RedisClient(const std::string& host, int port);
        ~RedisClient();

        /**
         * @brief Check if connected.
         */
        bool is_connected() const;

        /**
         * @brief Publish a message to a channel.
         *
         * @param channel The channel name (e.g., "sentry_alerts")
         * @param message The JSON payload
         */
        void publish(const std::string& channel, const std::string& message);

    private:
        /**
         * @brief Internal helper to reconnect if connection dropped.
         */
        bool connect();

        std::string host_;
        int port_;

        redisContext* context_ = nullptr;
        std::mutex mutex_; // hiredis is not thread-safe by default
        bool connected_ = false;
    };

} // namespace blackbox::storage

#endif // BLACKBOX_STORAGE_REDIS_CLIENT_H