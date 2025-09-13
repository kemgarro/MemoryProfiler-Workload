# Memory Profiler Workload

A comprehensive C++17 memory workload designed to stress test memory profilers and allocation tracking systems. This workload generates various memory allocation patterns including bursts, leaks, fragmentation, and complex data structures.

## Overview

This workload is specifically designed to test memory profilers that:
- Overload global `::operator new/delete/new[]/delete[]`
- Expose an API in namespace `mp::api` with functions like `getMetricsJson()` and `getSnapshotJson()`
- Can send data to a GUI via socket (the profiler handles networking, not this workload)

## Requirements

- **Compiler**: g++ ≥ 10 or compatible C++17 compiler
- **CMake**: ≥ 3.16
- **OS**: Linux (tested on Ubuntu 20.04+)

## Build Instructions

### Basic Build
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build . -j
```

### Build with Profiler API Support
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMP_USE_API=ON -DMP_MAX_MEM_MB=300 ..
cmake --build . -j
```

### Build Options

- `MP_USE_API` (ON/OFF, default OFF): Enable profiler API calls for periodic snapshots
- `MP_MAX_MEM_MB` (integer, default 300): Soft limit for memory usage planning
- `CMAKE_BUILD_TYPE`: Use `RelWithDebInfo` for debugging or `Release` for performance

## Usage

### Basic Usage
```bash
./mp_workload --threads 4 --seconds 8 --scale 1.5 --leak-rate 0.08 --burst-size 800
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--threads <N>` | Number of worker threads | 2 |
| `--seconds <S>` | Duration in seconds | 6 |
| `--seed <UINT>` | Random seed for reproducible runs | 12345 |
| `--scale <K>` | Scale factor for sizes and repetitions | 1.0 |
| `--leak-rate <p>` | Fraction of allocations that leak (0.0-1.0) | 0.05 |
| `--burst-size <B>` | Size of allocation bursts | 500 |
| `--no-leaks` | Disable memory leaks entirely | false |
| `--quiet` | Reduce log output | false |
| `--snapshot-every-ms <M>` | Snapshot interval (only with MP_USE_API) | 1000 |
| `--help` | Show help message | - |

### Example Commands

#### High-intensity workload
```bash
./mp_workload --threads 6 --seconds 10 --scale 2.0 --leak-rate 0.1 --burst-size 1000
```

#### Leak-free testing
```bash
./mp_workload --no-leaks --threads 2 --seconds 5 --scale 0.5
```

#### Memory fragmentation test
```bash
./mp_workload --threads 4 --seconds 8 --scale 1.5 --burst-size 200
```

#### Long-running stability test
```bash
./mp_workload --threads 8 --seconds 60 --scale 1.0 --leak-rate 0.02 --quiet
```

## Memory Patterns

The workload implements five distinct memory stress patterns:

### 1. AllocStorm
- **Purpose**: Burst allocations with mixed object types
- **Patterns**: 
  - Geometric and pseudo-random allocation sizes
  - Mix of trivial objects and non-trivial `Blob` objects
  - Non-ordered deallocations (not LIFO/FIFO)
  - Short and medium-lived objects

### 2. LeakFactory
- **Purpose**: Controlled memory leaks for leak detection testing
- **Patterns**:
  - Configurable leak rate via `--leak-rate`
  - Mix of object types (simple, arrays, blobs)
  - Leaked pointers stored in global repository
  - Avoids double-free and use-after-free

### 3. Fragmenter
- **Purpose**: Memory fragmentation simulation
- **Patterns**:
  - Many small allocations interleaved with medium ones
  - Shuffled deallocation patterns
  - Oscillating active memory set
  - "Sawtooth" memory usage patterns

### 4. VectorChurn
- **Purpose**: STL container stress testing
- **Patterns**:
  - `std::vector<std::string>` creation/destruction
  - `std::vector<std::unique_ptr<T>>` operations
  - `reserve()` calls triggering reallocations
  - Nested vector structures

### 5. TreeFactory
- **Purpose**: Complex data structure allocation
- **Patterns**:
  - Balanced and unbalanced binary trees
  - Exception injection during construction
  - Deep and wide tree structures
  - Proper cleanup with exceptions

## Output

### Normal Output
```
Starting memory workload with 4 threads for 8 seconds
Scale: 1.5, Leak rate: 0.08

Thread 0 AllocStorm: 1250 allocs, 980 deallocs, 15.2 MB total, 12 bursts
Thread 1 VectorChurn: 8 cycles, 8.7 MB total
Thread 2 Fragmenter: 1250 allocs, 980 deallocs, 450 peak active, 6 cycles
Thread 3 TreeFactory: 6 cycles, 1250 nodes created
Thread 4 LeakFactory: 45 leaks, 855 normal, 12.3 MB total

=== WORKLOAD SUMMARY ===
Configuration:
  Threads: 4
  Duration: 8s
  Scale: 1.5
  Leak rate: 0.08
  Burst size: 500

Memory Statistics:
  Total allocations: 12500
  Total deallocations: 9800
  Bytes allocated: 156.7 MB
  Bytes deallocated: 98.2 MB
  Peak memory estimate: 58.5 MB

Leak Statistics:
  Leaked objects: 45
  Leaked arrays: 12
  Total leaks: 57
  Leaked bytes: 8.4 MB

Performance:
  Total duration: 8003ms
  Allocations/sec: 1562

Module Breakdown:
  AllocStorm: 5000 allocs, 78.3 MB bytes, 2000ms
  LeakFactory: 2500 allocs, 39.2 MB bytes, 2000ms
  Fragmenter: 2500 allocs, 19.6 MB bytes, 2000ms
  VectorChurn: 1500 allocs, 12.3 MB bytes, 2000ms
  TreeFactory: 1000 allocs, 7.3 MB bytes, 2000ms
========================
```

### With Profiler API (MP_USE_API=ON)
```
SNAPSHOT: {"timestamp":1640995200000,"allocations":1250,"peak_memory":52428800}
SNAPSHOT: {"timestamp":1640995201000,"allocations":2500,"peak_memory":67108864}
...
```

## Integration with Memory Profilers

### Profiler API Integration
When built with `MP_USE_API=ON`, the workload will:
1. Check for `ProfilerAPI.hpp` using `__has_include`
2. Call `mp::api::getSnapshotJson()` periodically
3. Print snapshots to stdout with `SNAPSHOT:` prefix
4. Handle API failures gracefully

### Expected Profiler Behavior
The profiler should:
1. Overload global operators: `new`, `delete`, `new[]`, `delete[]`
2. Provide API functions in `mp::api` namespace
3. Track allocations, deallocations, and memory usage
4. Handle multi-threaded allocation patterns
5. Detect and report memory leaks

## Testing

### Smoke Test
```bash
cd tests
./smoke.sh
```

The smoke test:
1. Compiles the workload
2. Runs a 3-second test with 2 threads
3. Verifies output contains expected summary information
4. Returns 0 on success

### Manual Testing
```bash
# Test basic functionality
./mp_workload --seconds 3 --threads 2

# Test leak detection
./mp_workload --seconds 5 --leak-rate 0.1

# Test high memory usage
./mp_workload --seconds 10 --scale 3.0 --threads 6
```

## Architecture

### Threading Model
- Each worker thread runs all 5 modules in sequence
- Threads run for the specified duration
- Global leak repository is thread-safe
- No data races or undefined behavior

### Memory Management
- Uses only standard `new/delete` operators
- No custom allocators or placement new
- Respects `MP_MAX_MEM_MB` limit
- Proper exception safety

### Configuration
- All parameters configurable via command line
- Deterministic behavior with fixed seeds
- Scalable workload intensity

## Troubleshooting

### Common Issues

1. **Compilation errors**: Ensure C++17 support and CMake ≥ 3.16
2. **High memory usage**: Reduce `--scale` or `--threads`
3. **API not found**: Check `MP_USE_API` and `ProfilerAPI.hpp` availability
4. **Performance issues**: Use `--quiet` flag to reduce I/O

### Debug Build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DMP_USE_API=ON ..
cmake --build . -j
gdb ./mp_workload
```

## License

This workload is provided as-is for testing memory profilers. No warranty or support is provided.

## Contributing

This is a test workload - modifications should maintain compatibility with the specified memory profiler interface.
