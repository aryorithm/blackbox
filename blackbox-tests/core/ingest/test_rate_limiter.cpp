#include <gtest/gtest.h>
#include "blackbox/ingest/rate_limiter.h"
#include <thread>
#include <chrono>

class RateLimiterTest : public ::testing::Test {
    // Helper to reset singleton state if possible,
    // but since RateLimiter is a Singleton, we must assume shared state.
    // We use unique IPs to ensure isolation between tests.
};

TEST_F(RateLimiterTest, ShouldAllowBurst) {
    auto& limiter = blackbox::ingest::RateLimiter::instance();
    std::string ip = "10.0.0.1";

    // The default burst is usually 500.
    // We should be able to send 10 packets instantly.
    for(int i = 0; i < 10; ++i) {
        EXPECT_TRUE(limiter.should_allow(ip)) << "Packet " << i << " blocked unexpectedly";
    }
}

TEST_F(RateLimiterTest, ShouldBlockExcessiveTraffic) {
    auto& limiter = blackbox::ingest::RateLimiter::instance();
    std::string ip = "10.0.0.2";

    // Drain the bucket (assuming burst is ~500)
    int allowed = 0;
    for(int i = 0; i < 1000; ++i) {
        if(limiter.should_allow(ip)) allowed++;
    }

    // We expect some to pass and some to fail
    EXPECT_LT(allowed, 1000) << "Rate limiter did not block any packets";

    // Immediate subsequent packet should fail
    EXPECT_FALSE(limiter.should_allow(ip)) << "Bucket should be empty";
}

TEST_F(RateLimiterTest, ShouldRefillOverTime) {
    auto& limiter = blackbox::ingest::RateLimiter::instance();
    std::string ip = "10.0.0.3";

    // 1. Drain bucket
    while(limiter.should_allow(ip));
    EXPECT_FALSE(limiter.should_allow(ip));

    // 2. Wait (e.g., 100ms)
    // Refill rate is 100/sec -> 0.1/ms -> 10 tokens in 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 3. Should allow again
    EXPECT_TRUE(limiter.should_allow(ip)) << "Bucket did not refill after wait";
}