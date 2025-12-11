/**
 * @file clickhouse_client.h
 * @brief Lightweight HTTP wrapper for ClickHouse interaction.
 */

#ifndef BLACKBOX_STORAGE_CLICKHOUSE_CLIENT_H
#define BLACKBOX_STORAGE_CLICKHOUSE_CLIENT_H

#include <string>
#include <vector>
#include "blackbox/storage/storage_engine.h" // For DBRow definition

namespace blackbox::storage {

    class ClickHouseClient {
    public:
        /**
         * @brief Initialize the client.
         * @param host The hostname (e.g., "http://localhost:8123")
         */
        explicit ClickHouseClient(std::string host);
        ~ClickHouseClient();

        /**
         * @brief Executes a batch INSERT query.
         * 
         * Formats the C++ structs into a SQL INSERT statement 
         * and sends it via HTTP POST.
         * 
         * @param rows The batch of data to write
         * @return true if HTTP 200 OK, false otherwise
         */
        bool insert_logs(const std::vector<DBRow>& rows);

    private:
        std::string host_;

        // Helper to escape strings for SQL (prevent injection/errors)
        std::string escape_string(const std::string& input);
    };

} // namespace blackbox::storage

#endif // BLACKBOX_STORAGE_CLICKHOUSE_CLIENT_H