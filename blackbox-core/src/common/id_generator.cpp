
/**
 * @file id_generator.cpp
 * @brief Implementation of Lock-Free UUID Generation.
 */

#include "blackbox/common/id_generator.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace blackbox::common {

    // =========================================================
    // Generate UUID v4
    // =========================================================
    std::string IdGenerator::generate_uuid_v4() {
        // 1. Thread-Local Random Engine
        // Initialization happens only once per thread.
        // This avoids the expensive std::random_device() call on every log.
        static thread_local std::mt19937_64 engine{std::random_device{}()};
        static thread_local std::uniform_int_distribution<uint64_t> dist;

        // 2. Generate 128 bits of random data (2 x 64-bit integers)
        uint64_t part1 = dist(engine);
        uint64_t part2 = dist(engine);

        // 3. Apply UUID v4 Variant/Version bits
        // Part 1: ...-4xxx-... (Version 4)
        // We need to set the 4 bits at the 13th nibble (index 12 in hex)
        // In uint64 logic, this depends on byte ordering, but here we manipulate bytes conceptually.

        // A simpler way with string formatting:
        // We format the random numbers as hex and patch the specific characters.

        std::stringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(16) << part1
           << std::setw(16) << part2;

        std::string hex = ss.str(); // 32 chars

        // UUID Format: 8-4-4-4-12
        // Indices:     01234567-8901-2345-6789-012345678901

        // Set Version 4 (at index 12 of the 32-char string, which corresponds to the 3rd group)
        // Actually, let's construct it manually to be precise with bits.

        // Alternative Approach: Byte manipulation
        union {
            uint64_t ints[2];
            uint8_t bytes[16];
        } uuid;

        uuid.ints[0] = part1;
        uuid.ints[1] = part2;

        // Set Version: 4
        uuid.bytes[6] = (uuid.bytes[6] & 0x0F) | 0x40;

        // Set Variant: 10xxxxxx (RFC 4122)
        uuid.bytes[8] = (uuid.bytes[8] & 0x3F) | 0x80;

        // 4. Format to String
        // We use a small buffer to avoid stream overhead
        char buffer[37];
        snprintf(buffer, sizeof(buffer),
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uuid.bytes[0], uuid.bytes[1], uuid.bytes[2], uuid.bytes[3],
            uuid.bytes[4], uuid.bytes[5],
            uuid.bytes[6], uuid.bytes[7],
            uuid.bytes[8], uuid.bytes[9],
            uuid.bytes[10], uuid.bytes[11], uuid.bytes[12], uuid.bytes[13], uuid.bytes[14], uuid.bytes[15]
        );

        return std::string(buffer);
    }

} // namespace blackbox::common