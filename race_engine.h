#pragma once

#include "telemetry_data.h"
#include "shared_state.h"
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

namespace f1sim {

// ============================================================================
// Physics Constants (Basic)
// ============================================================================

constexpr float SIMULATION_HZ = 50.0f;
constexpr float DT = 1.0f / SIMULATION_HZ;  // 0.02 seconds per tick
constexpr float BASE_SPEED_KMH = 200.0f;     // Simple constant speed for now

// TODO: Add realistic physics constants:
// - Acceleration, braking, drag
// - Tire degradation rates
// - Fuel consumption
// - Aerodynamic effects

// ============================================================================
// Race Engine - The Producer Thread
// ============================================================================

class RaceEngine {
public:
    RaceEngine(SharedRaceState& shared_state, uint32_t seed, uint16_t total_laps)
        : shared_state_(shared_state)
        , rng_(seed)
        , total_laps_(total_laps)
        , tick_count_(0)
    {
        initialize_race();
    }

    // Main simulation loop (runs at 50Hz)
    void run() {
        using clock = std::chrono::steady_clock;
        
        auto next_tick = clock::now();
        const auto tick_duration = std::chrono::duration_cast<clock::duration>(
            std::chrono::duration<double>(DT)
        );

        while (!shared_state_.should_stop()) {
            // TODO: Add proper physics update
            update_simulation();
            
            // Publish state to consumer
            shared_state_.write_state(state_);
            
            // Check if race is complete
            if (is_race_complete()) {
                break;
            }
            
            // Precise timing - sleep until next tick
            next_tick += tick_duration;
            std::this_thread::sleep_until(next_tick);
        }
    }

private:
    void initialize_race() {
        state_ = RaceState{};
        state_.tick_count = 0;
        state_.race_time = 0.0f;
        
        // Initialize each car with basic starting positions
        for (size_t i = 0; i < NUM_DRIVERS; ++i) {
            auto& car = state_.cars[i];
            car.distance = -25.0f * static_cast<float>(i);  // Staggered start
            car.position = static_cast<uint8_t>(i + 1);
            car.current_lap = 1;
            car.speed = 0.0f;
            
            // TODO: Initialize more fields as you add them
        }
    }

    void update_simulation() {
        tick_count_++;
        state_.tick_count = tick_count_;
        state_.race_time += DT;
        
        // Simple physics update for each car
        for (size_t i = 0; i < NUM_DRIVERS; ++i) {
            update_car_physics(i);
        }
        
        // Update race order
        update_race_order();
    }

    void update_car_physics(size_t idx) {
        auto& car = state_.cars[idx];
        
        // TODO: Implement realistic physics here!
        // For now, just constant speed movement
        
        // Simple constant speed (with slight variation per car)
        float speed_variance = 1.0f + (static_cast<float>(idx) * 0.01f);
        car.speed = BASE_SPEED_KMH * speed_variance;
        
        // Update position based on speed
        float speed_ms = car.speed / 3.6f;  // km/h to m/s
        car.distance += speed_ms * DT;
        
        // Check lap completion
        if (car.distance >= TRACK_LENGTH * car.current_lap) {
            car.current_lap++;
        }
        
        // TODO: Add proper physics:
        // - Acceleration/braking
        // - Tire wear
        // - Fuel consumption
        // - Aerodynamics
        // - Collisions
    }

    void update_race_order() {
        // Sort cars by distance traveled
        std::array<size_t, NUM_DRIVERS> indices;
        for (size_t i = 0; i < NUM_DRIVERS; ++i) {
            indices[i] = i;
        }
        
        std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
            return state_.cars[a].distance > state_.cars[b].distance;
        });
        
        // Update positions
        for (size_t i = 0; i < NUM_DRIVERS; ++i) {
            size_t car_idx = indices[i];
            state_.cars[car_idx].position = static_cast<uint8_t>(i + 1);
        }
        
        // TODO: Calculate gaps, intervals, etc.
    }

    bool is_race_complete() const {
        // Race complete when leader finishes target laps
        return state_.cars[0].position == 1 && state_.cars[0].current_lap > total_laps_;
    }

private:
    SharedRaceState& shared_state_;
    RaceState state_;
    std::mt19937 rng_;  // For deterministic randomness
    uint16_t total_laps_;
    uint64_t tick_count_;
};

} // namespace f1sim
