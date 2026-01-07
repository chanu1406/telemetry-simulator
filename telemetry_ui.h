#pragma once

#include "telemetry_data.h"
#include "ring_buffer.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <array>
#include <thread>
#include <chrono>
#include <atomic>

namespace f1sim {

// TODO: Add fancy ANSI escape sequences for:
// - Colors
// - Cursor positioning
// - Screen clearing
// - Progress bars
// - Multiple pages/views

class TelemetryUI {
public:
    TelemetryUI(RingBuffer<TelemetryFrame>& ring_buffer, std::atomic<bool>& stop_flag)
        : ring_buffer_(ring_buffer)
        , stop_flag_(stop_flag)
    {
        std::cout << "\n=== F1 Telemetry Simulator ===\n\n";
        
        // Initialize car tracking array
        for (auto& car : latest_frames_) {
            car.driver_id = 255;  // Invalid marker
        }
    }

    void run() {
        while (!stop_flag_.load(std::memory_order_acquire)) {
            TelemetryFrame frame;
            
            // Blocking pop - waits for data
            if (!ring_buffer_.pop(frame)) {
                // Ring buffer shutdown
                break;
            }
            
            // Update latest frame for this driver
            latest_frames_[frame.driver_id] = frame;
            
            // Render at reduced rate (only on driver 0's frames for simplicity)
            if (frame.driver_id == 0) {
                render_frame();
            }
        }
        
        std::cout << "\n\nRace Complete!\n";
    }

private:
    void render_frame() {
        // Find leader (position 1)
        const TelemetryFrame* leader = nullptr;
        const TelemetryFrame* fastest = nullptr;
        
        for (const auto& frame : latest_frames_) {
            if (frame.driver_id == 255) continue;
            
            if (frame.position == 1) {
                leader = &frame;
            }
            // Also track driver 19 (fastest car)
            if (frame.driver_id == 19) {
                fastest = &frame;
            }
        }
        
        if (!leader) return;
        
        // Simple console output (TODO: Make this fancier!)
        std::cout << "\r";  // Carriage return (same line)
        std::cout << "Time: " << std::fixed << std::setprecision(1) 
                  << (leader->timestamp_ms / 1000.0f) << "s  ";
        std::cout << "Leader: P" << static_cast<int>(leader->position) 
                  << " (ID:" << static_cast<int>(leader->driver_id) << ") "
                  << "Speed: " << std::setw(3) << static_cast<int>(leader->speed) << " km/h  "
                  << "Lap: " << leader->lap 
                  << " Sector: " << static_cast<int>(leader->sector);
        
        if (fastest) {
            std::cout << "  [D19: Lap " << fastest->lap << "]";
        }
        
        std::cout << std::flush;
        
        // TODO: Add full leaderboard display
        // TODO: Add detailed telemetry (throttle, brake, tire wear)
        // TODO: Add sector times
        // TODO: Add strategy information
    }

private:
    RingBuffer<TelemetryFrame>& ring_buffer_;
    std::atomic<bool>& stop_flag_;
    std::array<TelemetryFrame, NUM_DRIVERS> latest_frames_;
};

} // namespace f1sim
