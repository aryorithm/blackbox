/**
 * @file inference_engine.h
 * @brief Wrapper around the proprietary xInfer library.
 * 
 * This class runs on the AI Worker Thread.
 * It manages the GPU context and executes the model.
 */

#ifndef BLACKBOX_ANALYSIS_INFERENCE_ENGINE_H
#define BLACKBOX_ANALYSIS_INFERENCE_ENGINE_H

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <optional>

// Forward declaration of your xInfer classes to avoid pulling in CUDA headers here
// This speeds up compilation of main.cpp significantly.
namespace xInfer {
    class Engine; 
    class Context;
}

namespace blackbox::analysis {

    class InferenceEngine {
    public:
        /**
         * @brief Construct a new Inference Engine.
         * 
         * @param model_path Path to the .plan file (TensorRT Engine)
         */
        explicit InferenceEngine(const std::string& model_path);
        
        ~InferenceEngine();

        /**
         * @brief Run inference on a single log vector.
         * 
         * @note In a real production scenario, you would implement a 
         * `evaluate_batch(std::vector<...>)` method to saturate the GPU.
         * For the MVP/simplicity, we show single-item inference.
         * 
         * @param input_vector The 128-float embedding from the Parser
         * @return float The anomaly score (0.0 = Safe, 1.0 = Critical)
         */
        float evaluate(const std::array<float, 128>& input_vector);

    private:
        // Pimpl idiom (Pointer to Implementation) or just holding the xInfer pointer
        std::unique_ptr<xInfer::Engine> engine_;
        std::unique_ptr<xInfer::Context> context_;

        // GPU Memory Pointers
        void* d_input_;  // Device Input Buffer
        void* d_output_; // Device Output Buffer
        
        // Caching dimensions
        size_t input_size_bytes_;
        size_t output_size_bytes_;
    };

} // namespace blackbox::analysis

#endif // BLACKBOX_ANALYSIS_INFERENCE_ENGINE_H