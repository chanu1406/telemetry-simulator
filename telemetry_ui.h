#pragma once

#include "telemetry_data.h"
#include "shared_state.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <array>
#include <thread>
#include <chrono>

namespace f1sim {

// TODO: Add fancy ANSI escape sequences for:
// - Colors
// - Cursor positioning
// - Screen clearing
// - Progress bars
// - Multiple pages/views

class TelemetryUI {
public:
    TelemetryUI(SharedRaceState& shared_state)
        : shared_state_(shared_state)
    {
        std::cout << "\n=== F1 Telemetry Simulator ===\n\n";
    }

    void run() {
        while (!shared_state_.should_stop()) {
            RaceState state = shared_state_.read_state();
            render_frame(state);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10 FPS for now
        }
        
        std::cout << "\n\nRace Complete!\n";
    }

private:
    void render_frame(const RaceState& state) {
        // Simple console output (TODO: Make this fancier!)
        std::cout << "\r";  // Carriage return (same line)
        std::cout << "Race Time: " << std::fixed << std::setprecision(1) << state.race_time << "s  ";
        std::cout << "Ticks: " << state.tick_count << "  ";
        
        // Leader info
        const CarTelemetry* leader = nullptr;
        for (const auto& car : state.cars) {
            if (car.position == 1) {
                leader = &car;
                break;
            }
        }
        
        if (leader) {
            std::cout << "Leader: P" << static_cast<int>(leader->position) 
                     << " Speed: " << std::setw(3) << static_cast<int>(leader->speed) << " km/h  "
                     << "Lap: " << leader->current_lap;
        }
        
        std::cout << std::flush;
        
        // TODO: Add full leaderboard display
        // TODO: Add detailed telemetry (throttle, brake, tire wear)
        // TODO: Add sector times
        // TODO: Add strategy information
    }

private:
    SharedRaceState& shared_state_;
};

} // namespace f1sim
