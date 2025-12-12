/**
 * @file parser_engine.cpp
 * @brief Implementation of Zero-Copy Parsing.
 */

#include "blackbox/parser/parser_engine.h"
#include <functional> // for std::hash
#include <cstring>    // for strchr

namespace blackbox::parser {

    // =========================================================
    // Constructor
    // =========================================================
ParserEngine::ParserEngine() {
    tokenizer_.load_vocabulary("config/vocab.txt");
    scaler_.load_parameters("config/scaler_params.txt"); // Load params
}
    // =========================================================
    // Helper: Extract Field (Move cursor forward)
    // =========================================================
    // Reads word until space, returns view, advances cursor
    std::string_view ParserEngine::extract_field(std::string_view& cursor) {
        if (cursor.empty()) return {};

        // Find position of next space
        size_t space_pos = cursor.find(' ');

        if (space_pos == std::string_view::npos) {
            // End of string
            std::string_view field = cursor;
            cursor = {}; // Empty the cursor
            return field;
        }

        // Return the slice
        std::string_view field = cursor.substr(0, space_pos);
        
        // Advance cursor past the space
        cursor.remove_prefix(space_pos + 1);
        
        return field;
    }

    // =========================================================
    // Process (The Hot Path)
    // =========================================================
    ParsedLog ParserEngine::process(const ingest::LogEvent& raw_event) {
        ParsedLog output;
        output.timestamp = raw_event.timestamp_ns;

        // 1. Create a view over the raw C-array
        // This costs practically nothing (pointer + size)
        std::string_view cursor(raw_event.raw_data, raw_event.length);

        // 2. Parse RFC5424 Syslog (Simplified Logic for MVP)
        // Format: <PRI>VERSION TIMESTAMP HOST APP-NAME PROCID MSGID STRUCTURED-DATA MSG
        // Example: <34>1 2023-10-10T... mymachine sshd ... ... ... Failed password
        
        // Skip PRI <34>1
        size_t angle_bracket = cursor.find('>');
        if (angle_bracket != std::string_view::npos) {
            cursor.remove_prefix(angle_bracket + 1); // Skip >
            // Skip Version
            extract_field(cursor); 
        }

        // Extract Timestamp (Skip for now, we use ingestion time)
        extract_field(cursor);

        // Extract Host
        output.host = extract_field(cursor);

        // Extract Service (App-Name)
        output.service = extract_field(cursor);

        // Skip PROCID and MSGID
        extract_field(cursor); 
        extract_field(cursor);

        // The rest is the Message
        output.message = cursor;

        // 3. Vectorize (Prepare for AI)
        vectorize_text(output.message, output.embedding_vector);

        return output;
    }

    // =========================================================
    // Vectorize (The Hashing Trick)
    // =========================================================
    void ParserEngine::vectorize_text(std::string_view text, std::array<float, 128>& out_vector) {
        // Clear vector
        out_vector.fill(0.0f);

        // Simple Feature Hashing (MurmurHash style logic simplified)
        // We hash chunks of the string and map them to indices 0..127
        
        std::hash<std::string_view> hasher;
        
        // A moving window over the text would be better, but for MVP:
        // We just hash the whole string and seed a few positions
        // In a real NLP model, we would loop over tokens.
        
        size_t h = hasher(text);
        
        // Spread the hash bits across the vector (Pseudo-embedding)
        for (int i = 0; i < 128; ++i) {
            if ((h >> i) & 1) {
                out_vector[i] = 1.0f;
            } else {
                out_vector[i] = 0.0f;
            }
        }
        
        // NOTE: In Phase 2, this function is replaced by a "vocab.json" lookup
        // that matches exactly what `blackbox-sim` does in Python.
    }

} // namespace blackbox::parser