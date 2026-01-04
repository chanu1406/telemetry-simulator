# Architecture

## System Design

```
Producer Thread (50Hz)    Consumer Thread (Variable)
       |                          |
       v                          v
   [Physics]                   [TUI]
       |                          |
       +----> [Shared State] <----+
              [Double Buffer]
              [Mutex + CV]
```

## Data Flow

**Producer:**
1. Compute new state
2. Lock mutex
3. Write to back buffer
4. Unlock and notify

**Consumer:**
1. Wait on condition variable
2. Lock mutex
3. Swap buffers
4. Unlock
5. Render from front buffer

## Memory Layout

```
CarTelemetry: 64 bytes (cache-aligned)
20 cars × 64 bytes = 1280 bytes total
Fits in L1 cache
```

## Thread Safety

- Mutex protects buffer swap
- Condition variable prevents busy-waiting
- No data races, no torn reads
- Deterministic with seeded RNG

## Extension Points

**Add telemetry field:**
1. Update CarTelemetry struct
2. Adjust padding for 64-byte alignment
3. Initialize in race_engine.h
4. Display in telemetry_ui.h

**Add physics:**
1. Add constants to race_engine.h
2. Implement in update_car_physics()
3. Test with multiple seeds
