/**
 * @file clickhouse_client.cpp
 * @brief Implementation of HTTP POST to ClickHouse.
 */

#include "blackbox/storage/clickhouse_client.h"
#include "blackbox/common/logger.h"
#include "blackbox/common/string_utils.h"
#include "blackbox/common/time_utils.h"
#include "blackbox/common/metrics.h"
#include <iostream>
#include <sstream>
#include <curl/curl.h>

namespace blackbox::storage {

    // =========================================================
    // Constructor
    // =========================================================
    ClickHouseClient::ClickHouseClient(std::string host) 
        : host_(std::move(host)) 
    {
        // Global init should theoretically happen once in main,
        // but it's safe to call multiple times if handled carefully.
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ClickHouseClient::~ClickHouseClient() {
        curl_global_cleanup();
    }

    // =========================================================
    // Insert Logs
    // =========================================================
    bool ClickHouseClient::insert_logs(const std::vector<DBRow>& rows) {
        if (rows.empty()) return true;

        // 1. Construct SQL
        // Table: sentry.logs
        std::stringstream sql;
        sql << "INSERT INTO sentry.logs (id, timestamp, host, country, service, message, anomaly_score, is_threat) VALUES ";

        bool first = true;
        for (const auto& row : rows) {
            if (!first) sql << ",";
            first = false;

            // Format Timestamp (ns -> YYYY-MM-DD HH:MM:SS)
            // Note: DBRow timestamp is uint64 ns. TimeUtils expects ms.
            std::string time_str = common::TimeUtils::to_clickhouse_format(row.timestamp / 1000000);

            // Escape Strings
            std::string safe_host = common::StringUtils::escape_sql(row.host);
            std::string safe_country = common::StringUtils::escape_sql(row.country);
            std::string safe_service = common::StringUtils::escape_sql(row.service);
            std::string safe_msg = common::StringUtils::escape_sql(row.message);

            sql << "("
                << "'" << row.id << "', "                 // UUID
                << "'" << time_str << "', "               // DateTime
                << "'" << safe_host << "', "              // Host/IP
                << "'" << safe_country << "', "           // Country Code
                << "'" << safe_service << "', "           // Service
                << "'" << safe_msg << "', "               // Message
                << row.anomaly_score << ", "              // Float
                << (row.is_alert ? 1 : 0)                 // UInt8
                << ")";
        }

        std::string query_data = sql.str();

        // 2. Setup CURL
        CURL* curl = curl_easy_init();
        if (!curl) {
            common::Metrics::instance().inc_db_errors(1);
            return false;
        }

        struct curl_slist* headers = nullptr;
        // Optional: headers = curl_slist_append(headers, "Content-Type: text/plain");

        curl_easy_setopt(curl, CURLOPT_URL, host_.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query_data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, query_data.size());

        // Fast Timeout (Prevent pipeline stall)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L);

        // 3. Execute
        CURLcode res = curl_easy_perform(curl);
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        bool success = true;

        if (res != CURLE_OK || response_code != 200) {
            LOG_ERROR("DB Write Failed. CURL Code: " + std::to_string(res) +
                      " HTTP: " + std::to_string(response_code));

            // Log the beginning of the query for debugging (truncated)
            LOG_DEBUG("Failed Query Start: " + query_data.substr(0, 100));

            common::Metrics::instance().inc_db_errors(1);
            success = false;
        } else {
            // Success
            // common::Metrics::instance().inc_db_rows_written(rows.size());
            // ^ handled in StorageEngine usually, but can be here too.
        }

        curl_easy_cleanup(curl);
        return success;
    }

} // namespace blackbox::storage