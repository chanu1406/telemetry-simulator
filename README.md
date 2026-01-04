# F1 Real-Time Telemetry Simulator

C++20 systems programming project: multi-threaded F1 race simulator with real-time telemetry.

**Status:** Foundation/skeleton with extension points. Many features are stubs marked with TODOs.

## Goals

- Low-level C++ (memory management, cache optimization)
- Multi-threading (Producer-Consumer architecture)
- Real-time systems (fixed timestep physics, deterministic replay)
- Performance engineering (zero-allocation hot paths)

## Implementation Status

**Complete:**
- Producer-Consumer threading (50Hz physics, variable UI rate)
- Double-buffered shared state (mutex + condition variable)
- Cache-line aligned telemetry structures
- Deterministic RNG
- Basic position tracking and lap counting

**Not Implemented:**

- Physics: realistic forces, aerodynamics, tire model, fuel, weather
- Track: segments with speed zones, DRS, pit lane, elevation
- AI: pit strategy, overtaking logic, defensive driving
- Visualization: ANSI colors, graphs, track map, replay
- Performance: lock-free structures, custom allocators, SIMD
- Networking: UDP export, WebSocket server

See TODO.md for detailed list.

## Structure

```
CPPtelemetry/
├── main.cpp              # Entry point
├── telemetry_data.h      # Data structures
├── shared_state.h        # Thread synchronization
├── race_engine.h         # Physics (Producer)
├── telemetry_ui.h        # UI (Consumer)
├── driver_stats.h        # TODO
├── track_model.h         # TODO
└── Makefile
```

## Build

```bash
make           # Release build
make debug     # Debug with sanitizers
make clean     # Remove artifacts
```

## Usage

```bash
./f1sim --seed 42 --laps 5
```

## Development

1. Pick a feature from TODO.md
2. Create feature branch
3. Implement incrementally
4. Test thoroughly (especially threading)
5. Commit with clear messages

## Performance Targets

- 50Hz physics with 20 cars
- Zero allocations in simulation loop
- < 10μs per car per tick
- Deterministic replay

## Documentation

- ARCHITECTURE.md: System design
- TODO.md: Feature roadmap
- QUICKSTART.md: Quick reference

## License

MIT
