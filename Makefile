# F1 Real-Time Telemetry Simulator
# Makefile for building high-performance C++20 executable

CXX = clang++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -Wpedantic -march=native
LDFLAGS = -pthread

TARGET = f1sim
SOURCES = main.cpp
HEADERS = telemetry_data.h shared_state.h race_engine.h telemetry_ui.h driver_stats.h track_model.h

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete: ./$(TARGET)"
	@echo "Run with: ./$(TARGET) --seed 42 --laps 5"

# Debug build
debug: CXXFLAGS = -std=c++20 -g -O0 -Wall -Wextra -Wpedantic -fsanitize=thread
debug: LDFLAGS = -pthread -fsanitize=thread
debug: $(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Run with default settings
run: $(TARGET)
	./$(TARGET)

# Run with custom seed
run-seed: $(TARGET)
	./$(TARGET) --seed 1337 --laps 3

# Check for memory issues (requires valgrind)
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) --laps 1

# Display help
help:
	@echo "F1 Telemetry Simulator - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build optimized release binary"
	@echo "  make debug    - Build with debug symbols and thread sanitizer"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make run      - Build and run with default settings"
	@echo "  make run-seed - Build and run with seed 1337"
	@echo "  make help     - Show this help message"

.PHONY: all debug clean run run-seed valgrind help
