#pragma once

#include "telemetry_data.h"
#include "season_data.h"
#include "ring_buffer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
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
        // Get driver info for name and team color
        const auto& driver_info = DRIVER_ROSTER[frame->driver_id];
        
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
        
        // Driver name with team color
        std::cout << driver_info.team_color << ANSIColor::BOLD 
                  << std::setw(14) << std::left << driver_info.name 
                  << ANSIColor::RESET << " ";
        
        // Pit indicator or progress bar
        if (frame->flags & FLAG_IN_PITS) {
            // Show pit stop status
            std::cout << ANSIColor::BRIGHT_YELLOW << "🔧 [IN PITS "
                      << std::fixed << std::setprecision(1) << frame->pit_timer 
                      << "s] " << ANSIColor::RESET;
        } else {
            // Progress bar (10 characters) showing lap completion
            float lap_progress = calculate_lap_progress(frame);
            std::cout << render_progress_bar(lap_progress, 10) << " ";
        }
        
        // Lap number
        std::cout << ANSIColor::CYAN << "Lap " << std::setw(2) << frame->lap << ANSIColor::RESET << "  ";
        
        // Gap to leader (or "LEADER" for P1)
        if (frame->position == 1) {
            std::cout << ANSIColor::BRIGHT_GREEN << "LEADER    " << ANSIColor::RESET;
        } else {
            const char* gap_color = frame->gap_to_leader < 5.0f ? ANSIColor::BRIGHT_YELLOW : ANSIColor::WHITE;
            std::cout << gap_color << "+" << std::fixed << std::setprecision(3) 
                      << std::setw(6) << frame->gap_to_leader << "s" << ANSIColor::RESET << " ";
        }
        
        // Speed (color-coded: green=fast, yellow=medium, red=slow)
        const char* speed_color = get_speed_color(frame->speed);
        std::cout << speed_color << std::setw(3) << static_cast<int>(frame->speed) 
                  << " km/h" << ANSIColor::RESET << "  ";
        
        // Tire wear (color-coded: green=fresh, yellow=worn, red=critical)
        const char* tire_color = get_tire_color(frame->tire_wear);
        std::cout << tire_color << "Tire: " << std::setw(2) << static_cast<int>(frame->tire_wear) 
                  << "%" << ANSIColor::RESET;
        
        // Pit stop count
        if (frame->pit_stops > 0) {
            std::cout << "  " << ANSIColor::MAGENTA << "Stops:" << frame->pit_stops << ANSIColor::RESET;
        }
        
        // Sector times (show if lap > 1, as we need at least one sector completion)
        if (frame->lap > 1 || frame->sector > 0) {
            std::cout << "  " << ANSIColor::GRAY << "[";
            
            // S1
            if (frame->sector_times[0] > 0) {
                std::cout << "S1:" << format_sector_time(frame->sector_times[0]);
            } else {
                std::cout << "S1:--.-";
            }
            
            std::cout << " ";
            
            // S2
            if (frame->sector_times[1] > 0) {
                std::cout << "S2:" << format_sector_time(frame->sector_times[1]);
            } else {
                std::cout << "S2:--.-";
            }
            
            std::cout << " ";
            
            // S3
            if (frame->sector_times[2] > 0) {
                std::cout << "S3:" << format_sector_time(frame->sector_times[2]);
            } else {
                std::cout << "S3:--.-";
            }
            
            std::cout << "]" << ANSIColor::RESET;
        }
        
        // Last lap time (show if we've completed at least one lap)
        if (frame->last_lap_time > 0) {
            std::cout << "  " << ANSIColor::BRIGHT_CYAN << "⏱ " 
                      << format_lap_time(frame->last_lap_time) << ANSIColor::RESET;
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
    
    std::string format_sector_time(uint32_t time_ms) {
        // Format: XX.X (e.g., 23.4)
        float seconds = time_ms / 1000.0f;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << seconds;
        return oss.str();
    }
    
    std::string format_lap_time(uint32_t time_ms) {
        // Format: M:SS.sss (e.g., 1:42.341)
        int minutes = time_ms / 60000;
        int seconds = (time_ms % 60000) / 1000;
        int milliseconds = time_ms % 1000;
        
        std::ostringstream oss;
        oss << minutes << ":" 
            << std::setfill('0') << std::setw(2) << seconds << "."
            << std::setfill('0') << std::setw(3) << milliseconds;
        return oss.str();
    }

private:
    RingBuffer<TelemetryFrame>& ring_buffer_;
    std::atomic<bool>& stop_flag_;
    std::array<TelemetryFrame, NUM_DRIVERS> latest_frames_;
    uint64_t frame_counter_;
};

} // namespace f1sim
