# Quick Reference

## Build & Run

```bash
make
./f1sim --seed 42 --laps 5
```

## Commands

```bash
make           # Release build
make debug     # Debug with sanitizers
make clean     # Clean
```

## Development

```bash
git checkout -b feature/tire-wear
# implement
git commit -m "Add tire wear"
```

## Debugging

```bash
# Thread sanitizer
make debug && ./f1sim --laps 1

# Profile (macOS)
instruments -t "Time Profiler" ./f1sim

# Profile (Linux)
perf record ./f1sim
perf report
```

## Code Style

- snake_case for variables/functions
- PascalCase for classes
- UPPER_CASE for constants
- No allocations in hot path
