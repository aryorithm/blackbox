/**
 * @file redis_client.cpp
 * @brief Implementation of Redis Publisher.
 */

#include "blackbox/storage/redis_client.h"
#include "blackbox/common/logger.h"
#include <hiredis/hiredis.h> // Requires libhiredis-dev
#include <iostream>

namespace blackbox::storage {

    // =========================================================
    // Constructor
    // =========================================================
    RedisClient::RedisClient(const std::string& host, int port)
        : host_(host), port_(port)
    {
        connect();
    }

    // =========================================================
    // Destructor
    // =========================================================
    RedisClient::~RedisClient() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (context_) {
            redisFree(context_);
            context_ = nullptr;
        }
    }

    // =========================================================
    // Connect Logic
    // =========================================================
    bool RedisClient::connect() {
        if (context_) {
            redisFree(context_);
            context_ = nullptr;
        }

        // Set timeout to 1.5 seconds (Fail fast)
        struct timeval timeout = { 1, 500000 };
        context_ = redisConnectWithTimeout(host_.c_str(), port_, timeout);

        if (context_ == nullptr || context_->err) {
            if (context_) {
                LOG_ERROR("Redis Connection Error: " + std::string(context_->errstr));
                redisFree(context_);
                context_ = nullptr;
            } else {
                LOG_ERROR("Redis Connection Error: Can't allocate context");
            }
            connected_ = false;
            return false;
        }

        LOG_INFO("Connected to Redis at " + host_ + ":" + std::to_string(port_));
        connected_ = true;
        return true;
    }

    bool RedisClient::is_connected() const {
        return connected_;
    }

    // =========================================================
    // Publish (The Hot Path)
    // =========================================================
    void RedisClient::publish(const std::string& channel, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 1. Auto-reconnect if needed
        if (!connected_ || !context_) {
            if (!connect()) {
                // Still failed, drop message
                return;
            }
        }

        // 2. Execute PUBLISH command
        // hiredis command format: "PUBLISH %s %s"
        auto reply = (redisReply*)redisCommand(context_, "PUBLISH %s %s", channel.c_str(), message.c_str());

        // 3. Check result
        if (reply == nullptr) {
            LOG_ERROR("Redis PUBLISH failed (Server disconnected?)");
            redisFree(context_);
            context_ = nullptr;
            connected_ = false;
            return;
        }

        // 4. Cleanup
        freeReplyObject(reply);
    }

} // namespace blackbox::storage