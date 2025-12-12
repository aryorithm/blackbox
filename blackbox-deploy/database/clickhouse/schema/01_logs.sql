-- 01_logs.sql
-- Create the main database
CREATE DATABASE IF NOT EXISTS sentry;

-- Create the heavy-lifting Log Table
CREATE TABLE IF NOT EXISTS sentry.logs
(
    -- 1. Identity
    id UUID,

    -- 2. Time
    timestamp DateTime64(3) CODEC(Delta, ZSTD(1)),

    -- 3. Source Metadata
    host String,
    country LowCardinality(String), -- e.g., 'US', 'CN', 'DE' (High compression)

    -- 4. Content
    service LowCardinality(String), -- e.g., 'sshd', 'nginx'
    message String CODEC(ZSTD(3)),  -- The raw log text, highly compressed

    -- 5. AI Enrichment
    anomaly_score Float32 CODEC(Gorilla), -- Gorilla codec is great for floats
    is_threat UInt8
)
ENGINE = MergeTree()
PARTITION BY toYYYYMMDD(timestamp) -- Daily partitions
ORDER BY (timestamp, service, host) -- Sort key for fast retrieval
TTL timestamp + INTERVAL 30 DAY;   -- Auto-delete data older than 30 days