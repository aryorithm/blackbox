/**
 * @file ring_buffer.cpp
 * @brief Implementation of Lock-Free Logic using Atomics.
 */

#include "blackbox/ingest/ring_buffer.h"
#include <cstring> // For std::memcpy
#include <chrono>

namespace blackbox::ingest {

    // =========================================================
    // Constructor
    // =========================================================
    template <size_t Capacity>
    RingBuffer<Capacity>::RingBuffer() : head_(0), tail_(0) {
        // Pre-allocate memory on startup. 
        // This prevents lag spikes during runtime.
        buffer_.resize(Capacity);
    }

    // =========================================================
    // Push (Producer)
    // =========================================================
    template <size_t Capacity>
    bool RingBuffer<Capacity>::push(const char* data, size_t len) {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        const size_t next_head = (current_head + 1) % Capacity;

        // Check if full
        // acquire: ensures we see the latest 'tail' update from the consumer
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false; 
        }

        // Write Data
        LogEvent& slot = buffer_[current_head];
        slot.timestamp_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        
        // Safety cap on size
        slot.length = (len > 4096) ? 4096 : len;
        
        // Fast memory copy
        std::memcpy(slot.raw_data, data, slot.length);

        // Commit the write
        // release: ensures the data write (memcpy) is visible before we update 'head'
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // =========================================================
    // Pop (Consumer)
    // =========================================================
    template <size_t Capacity>
    bool RingBuffer<Capacity>::pop(LogEvent& out_event) {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);

        // Check if empty
        // acquire: ensures we see the latest 'head' update from the producer
        if (current_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }

        // Read Data
        out_event = buffer_[current_tail];

        // Advance tail
        const size_t next_tail = (current_tail + 1) % Capacity;
        
        // Commit the read
        // release: ensures we are done reading before we update 'tail'
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // =========================================================
    // EXPLICIT INSTANTIATION
    // =========================================================
    // This tells the compiler: "Compile the code for RingBuffer<65536> RIGHT NOW."
    // Without this line, the Linker will give you "Undefined Reference" errors
    // because the logic is hidden in this .cpp file.
    
    template class RingBuffer<65536>;

} // namespace blackbox::ingest