#pragma once

#include "telemetry_data.h"
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace f1sim {

/**
 * Thread-safe double-buffered shared state for Producer-Consumer pattern.
 * Producer writes to back buffer, Consumer reads from front buffer.
 * Buffers are swapped atomically with proper synchronization.
 */
class SharedRaceState {
public:
    SharedRaceState() : has_new_data_(false), should_stop_(false) {
        // Initialize both buffers with zero state
        front_buffer_ = RaceState{};
        back_buffer_ = RaceState{};
    }

    /**
     * Producer: Write new race state (called at 50Hz by physics thread)
     * @param new_state The updated race state to publish
     */
    void write_state(const RaceState& new_state) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            back_buffer_ = new_state;
            has_new_data_ = true;
        }
        // Notify consumer that new data is ready
        cv_.notify_one();
    }

    /**
     * Consumer: Read latest race state (called by UI thread)
     * Blocks until new data is available.
     * @return The latest complete race state snapshot
     */
    RaceState read_state() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait for new data or stop signal
        cv_.wait(lock, [this] { 
            return has_new_data_ || should_stop_; 
        });
        
        if (should_stop_) {
            return front_buffer_; // Return last valid state
        }
        
        // Swap buffers - zero-copy handoff
        std::swap(front_buffer_, back_buffer_);
        has_new_data_ = false;
        
        return front_buffer_;
    }

    /**
     * Consumer: Try to read without blocking
     * @return pair<bool, RaceState> - true if new data was available
     */
    std::pair<bool, RaceState> try_read_state() {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        
        if (!lock.owns_lock() || !has_new_data_) {
            return {false, front_buffer_};
        }
        
        std::swap(front_buffer_, back_buffer_);
        has_new_data_ = false;
        
        return {true, front_buffer_};
    }

    /**
     * Signal all threads to stop (called on shutdown)
     */
    void signal_stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            should_stop_ = true;
        }
        cv_.notify_all();
    }

    bool should_stop() const {
        return should_stop_;
    }

private:
    // Double buffers for lock-free reads/writes
    RaceState front_buffer_;  // Consumer reads from this
    RaceState back_buffer_;   // Producer writes to this
    
    // Synchronization primitives
    std::mutex mutex_;
    std::condition_variable cv_;
    
    // State flags
    bool has_new_data_;
    std::atomic<bool> should_stop_;
};

} // namespace f1sim
