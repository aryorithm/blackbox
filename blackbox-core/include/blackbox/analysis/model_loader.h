/**
 * @file model_loader.h
 * @brief Binary File Loader for AI Models.
 * 
 * Handles the raw disk I/O required to load TensorRT engines (.plan)
 * or other binary artifacts into memory buffers.
 */

#ifndef BLACKBOX_ANALYSIS_MODEL_LOADER_H
#define BLACKBOX_ANALYSIS_MODEL_LOADER_H

#include <string>
#include <vector>
#include <cstdint>

namespace blackbox::analysis {

    class ModelLoader {
    public:
        /**
         * @brief Checks if a model file exists on disk.
         * @param path File path
         * @return true if exists
         */
        static bool exists(const std::string& path);

        /**
         * @brief Loads a binary file completely into memory.
         * 
         * TensorRT requires the whole model blob in RAM to deserialize it.
         * 
         * @param path File path
         * @return std::vector<char> Raw binary data
         * @throws std::runtime_error if file cannot be read
         */
        static std::vector<char> load_binary(const std::string& path);

        /**
         * @brief Get file size in bytes.
         * Useful for logging or allocation checks.
         */
        static size_t get_size(const std::string& path);
    };

} // namespace blackbox::analysis

#endif // BLACKBOX_ANALYSIS_MODEL_LOADER_H