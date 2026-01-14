#pragma once

#include <string>
#include <array>

namespace f1sim {

// 2025 F1 Driver and Team Data
struct DriverInfo {
    std::string name;
    std::string team;
    std::string team_color; // ANSI color code
};

// ANSI 256-color codes for authentic F1 team colors
namespace TeamColors {
    constexpr const char* RED_BULL = "\033[38;5;18m";      // Dark blue
    constexpr const char* FERRARI = "\033[38;5;196m";      // Bright red
    constexpr const char* MCLAREN = "\033[38;5;208m";      // Orange
    constexpr const char* MERCEDES = "\033[38;5;50m";      // Cyan/turquoise
    constexpr const char* ASTON_MARTIN = "\033[38;5;34m";  // Green
    constexpr const char* ALPINE = "\033[38;5;201m";       // Pink
    constexpr const char* RACING_BULLS = "\033[38;5;27m";  // Blue
    constexpr const char* HAAS = "\033[38;5;245m";         // Gray/white
    constexpr const char* WILLIAMS = "\033[38;5;33m";      // Light blue
    constexpr const char* KICK_SAUBER = "\033[38;5;46m";   // Bright green
}

// 2025 F1 Driver Roster (20 drivers across 10 teams)
constexpr std::array<DriverInfo, 20> DRIVER_ROSTER = {{
    // Red Bull Racing (Drivers 0-1)
    {"M. Verstappen", "Red Bull Racing", TeamColors::RED_BULL},
    {"S. Perez", "Red Bull Racing", TeamColors::RED_BULL},
    
    // Ferrari (Drivers 2-3)
    {"C. Leclerc", "Ferrari", TeamColors::FERRARI},
    {"L. Hamilton", "Ferrari", TeamColors::FERRARI},
    
    // McLaren (Drivers 4-5)
    {"L. Norris", "McLaren", TeamColors::MCLAREN},
    {"O. Piastri", "McLaren", TeamColors::MCLAREN},
    
    // Mercedes (Drivers 6-7)
    {"G. Russell", "Mercedes", TeamColors::MERCEDES},
    {"A. Antonelli", "Mercedes", TeamColors::MERCEDES},
    
    // Aston Martin (Drivers 8-9)
    {"F. Alonso", "Aston Martin", TeamColors::ASTON_MARTIN},
    {"L. Stroll", "Aston Martin", TeamColors::ASTON_MARTIN},
    
    // Alpine (Drivers 10-11)
    {"P. Gasly", "Alpine", TeamColors::ALPINE},
    {"J. Doohan", "Alpine", TeamColors::ALPINE},
    
    // Racing Bulls (Drivers 12-13)
    {"Y. Tsunoda", "Racing Bulls", TeamColors::RACING_BULLS},
    {"I. Hadjar", "Racing Bulls", TeamColors::RACING_BULLS},
    
    // Haas (Drivers 14-15)
    {"E. Ocon", "Haas F1 Team", TeamColors::HAAS},
    {"O. Bearman", "Haas F1 Team", TeamColors::HAAS},
    
    // Williams (Drivers 16-17)
    {"A. Albon", "Williams Racing", TeamColors::WILLIAMS},
    {"C. Sainz", "Williams Racing", TeamColors::WILLIAMS},
    
    // Kick Sauber (Drivers 18-19)
    {"N. Hulkenberg", "Kick Sauber", TeamColors::KICK_SAUBER},
    {"G. Bortoleto", "Kick Sauber", TeamColors::KICK_SAUBER}
}};

} // namespace f1sim
