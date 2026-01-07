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
// Core Telemetry Data
// ============================================================================

#pragma pack(push, 1)  // Packed for memory efficiency

/**
 * @brief Telemetry frame for a single car at one timestamp
 * 
 * Enhanced structure with fields needed for ring buffer streaming.
 * Still cache-line aligned for optimal performance.
 */
struct TelemetryFrame {
    // Timing & Identification
    uint32_t timestamp_ms;    // Race time in milliseconds (4 bytes)
    uint8_t driver_id;        // 0-19 (1 byte)
    uint8_t position;         // 1-20 (1 byte)
    uint16_t lap;             // Current lap (2 bytes)
    uint8_t sector;           // 0-2 (1 byte)
    
    // Motion Data
    float speed;              // km/h (4 bytes)
    float distance;           // Total distance traveled in meters (4 bytes)
    float throttle;           // 0.0 - 1.0 (4 bytes)
    
    // Tire Data (simplified for now)
    float tire_wear;          // 0.0 - 100.0% (4 bytes)
    
    // Status Flags (bitfield)
    uint8_t flags;            // IN_PITS=0x01, PENALTY=0x02, etc. (1 byte)
    
    // TODO: Add more fields as needed:
    // - float brake;
    // - float tire_temp_FL/FR/RL/RR;
    // - uint8_t gear;
    // - uint16_t engine_rpm;
    
    uint8_t padding[32];      // Pad to 64 bytes (4+1+1+2+1+4+4+4+4+1+32+6=64)
} __attribute__((aligned(64)));

// Status flag constants
constexpr uint8_t FLAG_IN_PITS   = 0x01;
constexpr uint8_t FLAG_PENALTY   = 0x02;
constexpr uint8_t FLAG_DNF       = 0x04;
constexpr uint8_t FLAG_SAFETY_CAR = 0x08;

/**
 * @brief Legacy structure for double-buffered state (kept for now)
 * 
 * Will be replaced by RingBuffer<TelemetryFrame> in Phase 2
 */
struct CarTelemetry {
    float speed;              // km/h (4 bytes)
    float distance;           // Total distance traveled in meters (4 bytes)
    uint8_t position;         // Race position (1-20) (1 byte)
    uint16_t current_lap;     // (2 bytes)
    uint8_t padding[53];      // Pad to 64 bytes total (4+4+1+2+53=64)
} __attribute__((aligned(64)));

struct RaceState {
    std::array<CarTelemetry, NUM_DRIVERS> cars;
    uint64_t tick_count;
    float race_time;          // seconds
};

#pragma pack(pop)

// Compile-time size verification
static_assert(sizeof(TelemetryFrame) == 64, "TelemetryFrame must be cache-line aligned (64 bytes)");
static_assert(sizeof(CarTelemetry) == 64, "CarTelemetry must be cache-line aligned (64 bytes)");

} // namespace f1sim
