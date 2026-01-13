#pragma once

#include "telemetry_data.h"
#include "ring_buffer.h"
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>

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
    RaceEngine(RingBuffer<TelemetryFrame>& ring_buffer, 
               std::atomic<bool>& stop_flag,
               uint32_t seed, 
               uint16_t total_laps)
        : ring_buffer_(ring_buffer)
        , stop_flag_(stop_flag)
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

        while (!stop_flag_.load(std::memory_order_acquire)) {
            // Update simulation
            update_simulation();
            
            // Push telemetry frames for each car to the ring buffer
            for (size_t i = 0; i < NUM_DRIVERS; ++i) {
                TelemetryFrame frame = create_frame(i);
                if (!ring_buffer_.push(frame)) {
                    // Ring buffer shutdown, exit
                    return;
                }
            }
            
            // Check if race is complete
            if (is_race_complete()) {
                stop_flag_.store(true, std::memory_order_release);
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
        
        // Initialize driver and car profiles
        initialize_profiles();
        
        // Initialize each car with basic starting positions
        for (size_t i = 0; i < NUM_DRIVERS; ++i) {
            auto& car_state = state_.cars[i];
            auto& telemetry = car_state.telemetry;
            
            telemetry.distance = -25.0f * static_cast<float>(i);  // Staggered start
            telemetry.position = static_cast<uint8_t>(i + 1);
            telemetry.current_lap = 1;
            telemetry.speed = 0.0f;
            
            // Initialize tire and pit stop state
            car_state.tire_wear = 0.0f;  // Fresh tires
            car_state.in_pits = false;
            car_state.pit_timer = 0.0f;
            car_state.pit_stops = 0;
            
            // Calculate pit threshold from driver profile
            const auto& profile = state_.driver_profiles[i];
            car_state.pit_threshold = 0.65f + (profile.tire_management * 0.25f);
            // Adjust by risk tolerance: risky drivers pit later, conservative earlier
            car_state.pit_threshold += (profile.risk_tolerance - 0.5f) * 0.15f;
            // Clamp to reasonable range [0.6, 0.95]
            car_state.pit_threshold = std::clamp(car_state.pit_threshold, 0.6f, 0.95f);
        }
    }
    
    void initialize_profiles() {
        // Team-based car profiles (10 teams, 2 drivers each)
        // Top teams: Red Bull, Ferrari, McLaren, Mercedes
        // Mid teams: Aston Martin, Alpine, Racing Bulls
        // Back teams: Haas, Williams, Kick Sauber
        
        std::array<CarProfile, 10> team_profiles = {{
            // Red Bull (0-1): Dominant car
            {0.95f, 0.95f, 0.92f, 0.94f},
            // Ferrari (2-3): Strong but slightly less reliable
            {0.93f, 0.92f, 0.90f, 0.88f},
            // McLaren (4-5): Balanced and improving
            {0.91f, 0.93f, 0.91f, 0.92f},
            // Mercedes (6-7): Strong engine, developing aero
            {0.94f, 0.88f, 0.89f, 0.93f},
            // Aston Martin (8-9): Mid-field leader
            {0.87f, 0.86f, 0.87f, 0.88f},
            // Alpine (10-11): Inconsistent but capable
            {0.84f, 0.85f, 0.83f, 0.82f},
            // Racing Bulls (12-13): Developing team
            {0.83f, 0.84f, 0.85f, 0.86f},
            // Haas (14-15): Budget constraints
            {0.80f, 0.81f, 0.82f, 0.84f},
            // Williams (16-17): Rebuilding
            {0.78f, 0.79f, 0.81f, 0.85f},
            // Kick Sauber (18-19): Back markers
            {0.76f, 0.77f, 0.80f, 0.83f}
        }};
        
        // Assign car profiles (2 drivers per team)
        for (size_t i = 0; i < NUM_DRIVERS; ++i) {
            size_t team_idx = i / 2;
            state_.car_profiles[i] = team_profiles[team_idx];
        }
        
        // Individual driver profiles with realistic variation
        // Format: {aggression, consistency, tire_management, risk_tolerance}
        state_.driver_profiles = {{
            // Red Bull Racing
            {0.85f, 0.97f, 0.90f, 0.75f},  // #0: Experienced champion (Max-like)
            {0.78f, 0.82f, 0.75f, 0.68f},  // #1: Talented but adapting (Lawson-like)
            
            // Ferrari
            {0.92f, 0.95f, 0.87f, 0.85f},  // #2: Aggressive star (Leclerc-like)
            {0.76f, 0.88f, 0.82f, 0.62f},  // #3: Legend adapting (Hamilton-like at Ferrari)
            
            // McLaren
            {0.84f, 0.94f, 0.88f, 0.72f},  // #4: Consistent fast driver (Norris-like)
            {0.72f, 0.91f, 0.86f, 0.65f},  // #5: Smart racer (Piastri-like)
            
            // Mercedes
            {0.88f, 0.91f, 0.84f, 0.78f},  // #6: Aggressive talent (Antonelli-like)
            {0.80f, 0.93f, 0.89f, 0.70f},  // #7: Experienced pro (Russell-like)
            
            // Aston Martin
            {0.86f, 0.89f, 0.83f, 0.74f},  // #8: Veteran (Alonso-like)
            {0.74f, 0.85f, 0.80f, 0.66f},  // #9: Solid #2 (Stroll-like)
            
            // Alpine
            {0.89f, 0.84f, 0.79f, 0.82f},  // #10: Fast but inconsistent (Gasly-like)
            {0.81f, 0.86f, 0.81f, 0.71f},  // #11: Rising talent (Doohan-like)
            
            // Racing Bulls
            {0.87f, 0.79f, 0.76f, 0.80f},  // #12: Aggressive youngster (Hadjar-like)
            {0.83f, 0.82f, 0.78f, 0.75f},  // #13: Developing driver
            
            // Haas
            {0.77f, 0.83f, 0.82f, 0.68f},  // #14: Solid midfielder (Ocon-like)
            {0.75f, 0.81f, 0.80f, 0.67f},  // #15: Experienced hand (Bearman-like)
            
            // Williams
            {0.82f, 0.80f, 0.77f, 0.76f},  // #16: Young charger (Sainz-like)
            {0.79f, 0.78f, 0.75f, 0.73f},  // #17: Rookie (Colapinto-like)
            
            // Kick Sauber
            {0.80f, 0.77f, 0.74f, 0.77f},  // #18: Experienced journeyman (Bottas-like)
            {0.76f, 0.76f, 0.73f, 0.72f}   // #19: Pay driver or rookie
        }};
    }
    
    TelemetryFrame create_frame(size_t car_idx) const {
        const auto& car_state = state_.cars[car_idx];
        const auto& telemetry = car_state.telemetry;
        
        TelemetryFrame frame{};
        frame.timestamp_ms = static_cast<uint32_t>(state_.race_time * 1000.0f);
        frame.driver_id = static_cast<uint8_t>(car_idx);
        frame.position = telemetry.position;
        frame.lap = telemetry.current_lap;
        frame.sector = calculate_sector(telemetry.distance, telemetry.current_lap);
        frame.speed = telemetry.speed;
        frame.distance = telemetry.distance;
        frame.throttle = car_state.in_pits ? 0.0f : 1.0f;  // No throttle in pits
        frame.tire_wear = car_state.tire_wear * 100.0f;  // Convert to percentage
        frame.flags = car_state.in_pits ? FLAG_IN_PITS : 0;
        
        return frame;
    }
    
    uint8_t calculate_sector(float distance, uint16_t lap) const {
        float lap_distance = distance - (TRACK_LENGTH * (lap - 1));
        float sector_length = TRACK_LENGTH / 3.0f;
        
        if (lap_distance < sector_length) return 0;
        if (lap_distance < sector_length * 2.0f) return 1;
        return 2;
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
        auto& car_state = state_.cars[idx];
        auto& telemetry = car_state.telemetry;
        const auto& driver = state_.driver_profiles[idx];
        const auto& car = state_.car_profiles[idx];
        
        // Handle pit stops
        if (car_state.in_pits) {
            car_state.pit_timer -= DT;
            if (car_state.pit_timer <= 0.0f) {
                // Exit pits
                car_state.in_pits = false;
                car_state.tire_wear = 0.0f;  // Fresh tires
                car_state.pit_stops++;
            }
            telemetry.speed = 0.0f;  // Stationary in pits
            return;
        }
        
        // Check if driver should pit
        if (car_state.tire_wear >= car_state.pit_threshold && !car_state.in_pits) {
            car_state.in_pits = true;
            // Pit stop duration: 2-3 seconds based on car reliability
            car_state.pit_timer = PIT_STOP_BASE_DURATION + (1.0f - car.reliability) * 0.5f;
            return;
        }
        
        // Calculate tire wear
        // Base wear affected by driver aggression and track characteristics
        float wear_rate = TIRE_WEAR_BASE_RATE * (1.0f + driver.aggression * 0.5f);
        // Tire management skill reduces wear
        wear_rate *= (1.0f - driver.tire_management * 0.3f);
        car_state.tire_wear += wear_rate * DT;
        car_state.tire_wear = std::min(car_state.tire_wear, 1.0f);
        
        // Calculate driver skill factor
        // Consistent drivers extract more performance from the car
        float driver_skill = 0.80f + driver.consistency * 0.25f;  // Range: [0.80, 1.05]
        
        // Calculate base speed from car performance
        float base_speed = BASE_SPEED_KMH * car.engine_power * driver_skill;
        
        // Apply tire wear penalty
        // Worn tires reduce speed (up to 30% reduction at 100% wear)
        float tire_factor = 1.0f - (car_state.tire_wear * 0.3f);
        
        // Add slight randomness for lap time variation based on consistency
        // Less consistent drivers have more variation
        std::uniform_real_distribution<float> speed_dist(-5.0f, 5.0f);
        float speed_variation = speed_dist(rng_) * (1.0f - driver.consistency);
        
        telemetry.speed = (base_speed * tire_factor) + speed_variation;
        telemetry.speed = std::max(telemetry.speed, 50.0f);  // Minimum speed
        
        // Update position based on speed
        float speed_ms = telemetry.speed / 3.6f;  // km/h to m/s
        telemetry.distance += speed_ms * DT;
        
        // Check lap completion
        if (telemetry.distance >= TRACK_LENGTH * telemetry.current_lap) {
            telemetry.current_lap++;
        }
    }

    void update_race_order() {
        // Sort cars by distance traveled
        std::array<size_t, NUM_DRIVERS> indices;
        for (size_t i = 0; i < NUM_DRIVERS; ++i) {
            indices[i] = i;
        }
        
        std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
            return state_.cars[a].telemetry.distance > state_.cars[b].telemetry.distance;
        });
        
        // Update positions
        for (size_t i = 0; i < NUM_DRIVERS; ++i) {
            size_t car_idx = indices[i];
            state_.cars[car_idx].telemetry.position = static_cast<uint8_t>(i + 1);
        }
    }

    bool is_race_complete() const {
        // Race complete when leader completes the final lap (starts lap total_laps + 1)
        for (const auto& car_state : state_.cars) {
            if (car_state.telemetry.position == 1 && car_state.telemetry.current_lap > total_laps_) {
                return true;
            }
        }
        return false;
    }

private:
    RingBuffer<TelemetryFrame>& ring_buffer_;
    std::atomic<bool>& stop_flag_;
    RaceState state_;
    std::mt19937 rng_;  // For deterministic randomness
    uint16_t total_laps_;
    uint64_t tick_count_;
};

} // namespace f1sim
