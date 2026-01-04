#include "race_engine.h"
#include "telemetry_ui.h"
#include "shared_state.h"
#include <iostream>
#include <thread>
#include <csignal>
#include <cstdlib>
#include <cstring>

using namespace f1sim;

// ============================================================================
// Global state for signal handling
// ============================================================================

static SharedRaceState* g_shared_state = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT && g_shared_state) {
        std::cout << "\n\nShutting down gracefully...\n";
        g_shared_state->signal_stop();
    }
}

// ============================================================================
// Command-line argument parsing
// ============================================================================

struct SimulationConfig {
    uint32_t seed = 42;
    uint16_t laps = 5;
    bool show_help = false;
};

SimulationConfig parse_arguments(int argc, char* argv[]) {
    SimulationConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            config.show_help = true;
        }
        else if (arg == "--seed" && i + 1 < argc) {
            config.seed = static_cast<uint32_t>(std::atoi(argv[++i]));
        }
        else if (arg == "--laps" && i + 1 < argc) {
            config.laps = static_cast<uint16_t>(std::atoi(argv[++i]));
        }
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            config.show_help = true;
        }
    }
    
    return config;
}

void print_usage(const char* program_name) {
    std::cout << "\n";
    std::cout << "F1 Real-Time Telemetry Simulator\n";
    std::cout << "=================================\n\n";
    std::cout << "A high-performance C++20 simulation demonstrating:\n";
    std::cout << "  • Producer-Consumer threading pattern\n";
    std::cout << "  • Zero-allocation hot path\n";
    std::cout << "  • Deterministic replay with RNG seeding\n";
    std::cout << "  • Real-time TUI with ANSI escape sequences\n";
    std::cout << "  • Cache-friendly memory layout\n\n";
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --seed N     Set random seed for deterministic replay (default: 42)\n";
    std::cout << "  --laps N     Set number of race laps (default: 5)\n";
    std::cout << "  --help, -h   Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " --seed 1337 --laps 10\n";
    std::cout << "  " << program_name << " --seed 999\n\n";
    std::cout << "Press Ctrl+C to stop the simulation.\n\n";
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    SimulationConfig config = parse_arguments(argc, argv);
    
    if (config.show_help) {
        print_usage(argv[0]);
        return 0;
    }
    
    // Display startup info
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  F1 Real-Time Telemetry Simulator                         ║\n";
    std::cout << "║  High-Performance C++20 Systems Programming Showcase      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Configuration:\n";
    std::cout << "  • Random Seed:    " << config.seed << "\n";
    std::cout << "  • Race Laps:      " << config.laps << "\n";
    std::cout << "  • Drivers:        " << NUM_DRIVERS << "\n";
    std::cout << "  • Physics Rate:   50 Hz (20ms per tick)\n";
    std::cout << "  • Track Length:   " << TRACK_LENGTH << " meters\n";
    std::cout << "\n";
    std::cout << "Starting simulation in 2 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Create shared state
    SharedRaceState shared_state;
    g_shared_state = &shared_state;
    
    // Setup signal handler for graceful shutdown
    std::signal(SIGINT, signal_handler);
    
    // Create engine and UI
    RaceEngine engine(shared_state, config.seed, config.laps);
    TelemetryUI ui(shared_state);
    
    // Launch threads
    std::thread producer_thread([&engine]() {
        engine.run();
    });
    
    std::thread consumer_thread([&ui]() {
        ui.run();
    });
    
    // Wait for threads to complete
    producer_thread.join();
    
    // Signal consumer to stop
    shared_state.signal_stop();
    consumer_thread.join();
    
    // Cleanup
    std::cout << "\nSimulation complete!\n";
    std::cout << "Seed used: " << config.seed << " (use this seed to replay exact race)\n\n";
    
    return 0;
}
