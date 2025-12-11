/**
 * @file clickhouse_client.cpp
 * @brief Implementation of HTTP POST to ClickHouse.
 */

#include "blackbox/storage/clickhouse_client.h"
#include <iostream>
#include <sstream>
#include <curl/curl.h> // Requires libcurl installed

namespace blackbox::storage {

    // =========================================================
    // Constructor
    // =========================================================
    ClickHouseClient::ClickHouseClient(std::string host) 
        : host_(std::move(host)) 
    {
        // Initialize global curl state (thread safety warning: do this once in main ideally)
        curl_global_init(CURL_GLOBAL_ALL);
    }

    // =========================================================
    // Destructor
    // =========================================================
    ClickHouseClient::~ClickHouseClient() {
        curl_global_cleanup();
    }

    // =========================================================
    // Helper: SQL Escape
    // =========================================================
    std::string ClickHouseClient::escape_string(const std::string& input) {
        std::string res;
        res.reserve(input.size());
        for (char c : input) {
            if (c == '\'') res += "\\'"; // Escape single quotes
            else if (c == '\\') res += "\\\\"; // Escape backslashes
            else res += c;
        }
        return res;
    }

    // =========================================================
    // Insert Logs (The Hot Path)
    // =========================================================
    bool ClickHouseClient::insert_logs(const std::vector<DBRow>& rows) {
        if (rows.empty()) return true;

        // 1. Construct the SQL Query
        // Format: INSERT INTO sentry.logs (timestamp, service, message, anomaly_score, is_threat) VALUES ...
        std::stringstream sql;
        sql << "INSERT INTO sentry.logs (timestamp, service, message, anomaly_score, is_threat) VALUES ";

        bool first = true;
        for (const auto& row : rows) {
            if (!first) sql << ",";
            first = false;

            // ClickHouse DateTime64 is typically an integer/string, 
            // but for simplicity here we assume the DB handles standard formatting or ints.
            // Note: Anomaly score is float, is_threat is 0 or 1.
            sql << "(" 
                << row.timestamp << ", " 
                << "'" << escape_string(row.service) << "', " 
                << "'" << escape_string(row.message) << "', " 
                << row.anomaly_score << ", " 
                << (row.is_alert ? 1 : 0) 
                << ")";
        }

        std::string query_data = sql.str();

        // 2. Perform HTTP POST via libcurl
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "[ERR] Failed to init curl" << std::endl;
            return false;
        }

        CURLcode res;
        struct curl_slist* headers = nullptr;

        // ClickHouse expects the query in the body
        curl_easy_setopt(curl, CURLOPT_URL, host_.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query_data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, query_data.size());

        // Fast Fail Settings (Don't hang the Storage Thread)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000L); // 1 second timeout

        // Perform request
        res = curl_easy_perform(curl);

        bool success = true;
        if (res != CURLE_OK) {
            std::cerr << "[ERR] ClickHouse Post Failed: " << curl_easy_strerror(res) << std::endl;
            success = false;
        } else {
            // Check HTTP code
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code != 200) {
                std::cerr << "[ERR] ClickHouse returned HTTP " << response_code << std::endl;
                success = false;
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);
        return success;
    }

} // namespace blackbox::storage