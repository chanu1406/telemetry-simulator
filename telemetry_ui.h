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
#include <cmath>

namespace f1sim {

// ============================================================================
// ANSI Color Codes
// ============================================================================

namespace ANSIColor {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    
    // Foreground colors
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* WHITE = "\033[37m";
    constexpr const char* GRAY = "\033[90m";
    
    // Bright colors
    constexpr const char* BRIGHT_RED = "\033[91m";
    constexpr const char* BRIGHT_GREEN = "\033[92m";
    constexpr const char* BRIGHT_YELLOW = "\033[93m";
    constexpr const char* BRIGHT_CYAN = "\033[96m";
    constexpr const char* BRIGHT_WHITE = "\033[97m";
    
    // Background colors
    constexpr const char* BG_RED = "\033[41m";
    constexpr const char* BG_GREEN = "\033[42m";
    constexpr const char* BG_YELLOW = "\033[43m";
    
    // Special
    constexpr const char* CLEAR_SCREEN = "\033[2J\033[H";
}

class TelemetryUI {
public:
    TelemetryUI(RingBuffer<TelemetryFrame>& ring_buffer, std::atomic<bool>& stop_flag)
        : ring_buffer_(ring_buffer)
        , stop_flag_(stop_flag)
        , frame_counter_(0)
    {
        std::cout << "\n" << ANSIColor::BOLD << ANSIColor::CYAN 
                  << "=== F1 Real-Time Telemetry Simulator ===" 
                  << ANSIColor::RESET << "\n\n";
        
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
            
            // Render at reduced rate - update every 10 frames (~5 Hz instead of 50 Hz)
            if (frame.driver_id == 0) {
                frame_counter_++;
                if (frame_counter_ % 10 == 0) {
                    render_leaderboard();
                }
            }
        }
        
        // Final leaderboard
        render_leaderboard();
        std::cout << "\n" << ANSIColor::BOLD << ANSIColor::GREEN 
                  << "🏁 Race Complete! 🏁" << ANSIColor::RESET << "\n\n";
    }

private:
    void render_leaderboard() {
        // Clear screen and move cursor to top
        std::cout << ANSIColor::CLEAR_SCREEN;
        
        // Sort drivers by position
        std::array<const TelemetryFrame*, NUM_DRIVERS> sorted_frames;
        for (size_t i = 0; i < NUM_DRIVERS; ++i) {
            sorted_frames[i] = &latest_frames_[i];
        }
        
        std::sort(sorted_frames.begin(), sorted_frames.end(), 
                  [](const TelemetryFrame* a, const TelemetryFrame* b) {
                      if (a->driver_id == 255) return false;
                      if (b->driver_id == 255) return true;
                      return a->position < b->position;
                  });
        
        const TelemetryFrame* leader = sorted_frames[0];
        if (leader->driver_id == 255) return;  // No data yet
        
        // Header
        float race_time = leader->timestamp_ms / 1000.0f;
        int minutes = static_cast<int>(race_time) / 60;
        int seconds = static_cast<int>(race_time) % 60;
        
        std::cout << ANSIColor::BOLD << ANSIColor::BRIGHT_YELLOW 
                  << "🏁 LAP " << leader->lap << " | Race Time: " 
                  << minutes << ":" << std::setfill('0') << std::setw(2) << seconds 
                  << " 🏁" << ANSIColor::RESET << "\n";
        std::cout << ANSIColor::GRAY << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << ANSIColor::RESET;
        
        // Leaderboard - show top 10 or all if <= 15
        size_t display_count = std::min(size_t(15), NUM_DRIVERS);
        
        for (size_t i = 0; i < display_count; ++i) {
            const TelemetryFrame* frame = sorted_frames[i];
            if (frame->driver_id == 255) continue;
            
            render_driver_row(frame, i);
        }
        
        std::cout << ANSIColor::GRAY << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << ANSIColor::RESET;
        std::cout << std::flush;
    }
    
    void render_driver_row(const TelemetryFrame* frame, size_t index) {
        // Position indicator with medal emojis for podium
        std::string position_icon;
        const char* position_color = ANSIColor::WHITE;
        
        if (frame->position == 1) {
            position_icon = "🥇";
            position_color = ANSIColor::BRIGHT_YELLOW;
        } else if (frame->position == 2) {
            position_icon = "🥈";
            position_color = ANSIColor::GRAY;
        } else if (frame->position == 3) {
            position_icon = "🥉";
            position_color = ANSIColor::YELLOW;
        } else {
            position_icon = "  ";
            position_color = ANSIColor::WHITE;
        }
        
        std::cout << position_icon << " " << position_color << ANSIColor::BOLD 
                  << "P" << std::setfill(' ') << std::setw(2) << static_cast<int>(frame->position) 
                  << ANSIColor::RESET << "  ";
        
        // Driver name/ID
        std::cout << "Driver " << std::setw(2) << static_cast<int>(frame->driver_id) << "  ";
        
        // Progress bar (10 characters) showing lap completion
        float lap_progress = calculate_lap_progress(frame);
        std::cout << render_progress_bar(lap_progress, 10) << "  ";
        
        // Lap number
        std::cout << ANSIColor::CYAN << "Lap " << std::setw(2) << frame->lap << ANSIColor::RESET << "   ";
        
        // Speed (color-coded: green=fast, yellow=medium, red=slow)
        const char* speed_color = get_speed_color(frame->speed);
        std::cout << speed_color << "Speed: " << std::setw(3) << static_cast<int>(frame->speed) 
                  << " km/h" << ANSIColor::RESET << "   ";
        
        // Tire wear (color-coded: green=fresh, yellow=worn, red=critical)
        const char* tire_color = get_tire_color(frame->tire_wear);
        std::cout << tire_color << "Tire: " << std::setw(2) << static_cast<int>(frame->tire_wear) 
                  << "%" << ANSIColor::RESET;
        
        // Pit stop indicator
        if (frame->flags & FLAG_IN_PITS) {
            std::cout << "  " << ANSIColor::MAGENTA << ANSIColor::BOLD 
                      << "[IN PITS]" << ANSIColor::RESET;
        }
        
        std::cout << "\n";
    }
    
    std::string render_progress_bar(float progress, int width) {
        int filled = static_cast<int>(progress * width);
        std::string bar = ANSIColor::GREEN;
        
        for (int i = 0; i < width; ++i) {
            if (i < filled) {
                bar += "█";
            } else {
                bar += ANSIColor::GRAY;
                bar += "░";
            }
        }
        
        bar += ANSIColor::RESET;
        return bar;
    }
    
    float calculate_lap_progress(const TelemetryFrame* frame) {
        float lap_distance = frame->distance - (TRACK_LENGTH * (frame->lap - 1));
        float progress = lap_distance / TRACK_LENGTH;
        return std::clamp(progress, 0.0f, 1.0f);
    }
    
    const char* get_speed_color(float speed) {
        if (speed >= 190.0f) return ANSIColor::BRIGHT_GREEN;
        if (speed >= 170.0f) return ANSIColor::YELLOW;
        return ANSIColor::RED;
    }
    
    const char* get_tire_color(float tire_wear) {
        if (tire_wear < 30.0f) return ANSIColor::BRIGHT_GREEN;
        if (tire_wear < 60.0f) return ANSIColor::YELLOW;
        return ANSIColor::BRIGHT_RED;
    }

private:
    RingBuffer<TelemetryFrame>& ring_buffer_;
    std::atomic<bool>& stop_flag_;
    std::array<TelemetryFrame, NUM_DRIVERS> latest_frames_;
    uint64_t frame_counter_;
};

} // namespace f1sim
