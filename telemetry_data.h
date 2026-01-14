#pragma once

#include <cstdint>
#include <array>

namespace f1sim {

// ============================================================================
// Driver & Car Profiles
// ============================================================================

/**
 * @brief Driver behavioral characteristics
 * 
 * All values in range [0.0, 1.0]
 */
struct DriverProfile {
    float aggression;        // Affects tire wear rate (0.0 = smooth, 1.0 = aggressive)
    float consistency;       // Affects lap time variance and skill factor (0.0 = erratic, 1.0 = consistent)
    float tire_management;   // Resistance to tire degradation (0.0 = poor, 1.0 = excellent)
    float risk_tolerance;    // Willingness to push limits (0.0 = conservative, 1.0 = risky)
};

/**
 * @brief Car performance characteristics
 * 
 * All values in range [0.0, 1.0] representing relative performance
 */
struct CarProfile {
    float engine_power;        // Top speed capability (0.0 = slowest, 1.0 = fastest)
    float aero_efficiency;     // Cornering and downforce (0.0 = poor, 1.0 = excellent)
    float cooling_efficiency;  // Tire temperature stability (0.0 = poor, 1.0 = excellent)
    float reliability;         // Affects pit stop duration and failure probability (0.0 = unreliable, 1.0 = bulletproof)
};

// ============================================================================
// Constants
// ============================================================================

constexpr size_t NUM_DRIVERS = 20;
constexpr float TRACK_LENGTH = 5000.0f;  // meters  
constexpr float TIRE_WEAR_BASE_RATE = 0.00125f;  // Base wear per second (~7.5% per lap, 1-2 stops per race)
constexpr float PIT_STOP_BASE_DURATION = 2.5f; // Base pit stop duration (seconds)

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
    
    // Pit Stop Data
    uint8_t pit_stops;        // Number of pit stops completed (1 byte)
    float pit_timer;          // Time remaining in pit (seconds, 0 if not in pits) (4 bytes)
    
    // Gap Data
    float gap_to_leader;      // Gap to P1 in seconds (negative if leader) (4 bytes)
    
    // Status Flags (bitfield)
    uint8_t flags;            // IN_PITS=0x01, PENALTY=0x02, etc. (1 byte)
    
    // Sector & Lap Timing
    uint32_t sector_times[3]; // S1, S2, S3 times in milliseconds (12 bytes)
    uint32_t last_lap_time;   // Previous lap time in milliseconds (4 bytes)
    
    // TODO: Add more fields as needed:
    // - float brake;
    // - float tire_temp_FL/FR/RL/RR;
    // - uint8_t gear;
    // - uint16_t engine_rpm;
    
    uint8_t padding[3];       // Pad to 64 bytes (4+1+1+2+1+4+4+4+4+1+4+4+1+12+4+3=64)
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
    float gap_to_leader;      // Gap to P1 in seconds (4 bytes)
    uint8_t padding[49];      // Pad to 64 bytes total (4+4+1+2+4+49=64)
} __attribute__((aligned(64)));

/**
 * @brief Extended car state with profile information
 */
struct CarState {
    CarTelemetry telemetry;   // Current telemetry data
    float tire_wear;          // Current tire wear (0.0 = fresh, 1.0 = worn out)
    float pit_threshold;      // When to pit (computed from driver profile)
    bool in_pits;             // Currently in pit stop
    float pit_timer;          // Time remaining in pit stop (seconds)
    uint8_t pit_stops;        // Number of pit stops completed
};

struct RaceState {
    std::array<CarState, NUM_DRIVERS> cars;
    std::array<DriverProfile, NUM_DRIVERS> driver_profiles;
    std::array<CarProfile, NUM_DRIVERS> car_profiles;
    uint64_t tick_count;
    float race_time;          // seconds
};

#pragma pack(pop)

// Compile-time size verification
static_assert(sizeof(TelemetryFrame) == 64, "TelemetryFrame must be cache-line aligned (64 bytes)");
static_assert(sizeof(CarTelemetry) == 64, "CarTelemetry must be cache-line aligned (64 bytes)");

} // namespace f1sim
