#pragma once

#include <cstdint>
#include <array>

namespace f1sim {

// ============================================================================
// Constants
// ============================================================================

constexpr size_t NUM_DRIVERS = 20;
constexpr float TRACK_LENGTH = 5000.0f;  // meters

// ============================================================================
// Core Telemetry Data (Simplified Skeleton)
// ============================================================================

// TODO: Add more fields as needed:
// - Tire data (compound, wear, temperature)
// - Engine data (RPM, temperature, fuel)
// - Aerodynamics (DRS, wing settings)
// - Timing splits (sector times)
// - Status flags (pit lane, yellow flag, etc.)

#pragma pack(push, 1)  // Packed for memory efficiency

struct CarTelemetry {
    float speed;              // km/h (4 bytes)
    float distance;           // Total distance traveled in meters (4 bytes)
    uint8_t position;         // Race position (1-20) (1 byte)
    uint16_t current_lap;     // (2 bytes)
    
    // TODO: Add more telemetry fields here
    // float throttle;
    // float brake;
    // uint8_t gear;
    // float tire_wear;
    // etc.
    
    uint8_t padding[53];      // Pad to 64 bytes total (4+4+1+2+53=64)
} __attribute__((aligned(64)));

struct RaceState {
    std::array<CarTelemetry, NUM_DRIVERS> cars;
    uint64_t tick_count;
    float race_time;          // seconds
};

#pragma pack(pop)

// Compile-time size verification
static_assert(sizeof(CarTelemetry) == 64, "CarTelemetry must be cache-line aligned (64 bytes)");

} // namespace f1sim
