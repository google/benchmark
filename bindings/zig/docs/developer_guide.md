# Developer Guide

## Project Structure

```
bindings/zig/
├── build.zig              # Zig build system
├── build.zig.zon          # Package metadata
├── src/
│   ├── zig_api.h          # C adapter header (extern "C" declarations)
│   ├── zig_api.cc         # C adapter implementation (C++ code)
│   ├── benchmark.zig      # Public Zig API
│   └── benchmark_test.zig # Unit tests
├── examples/
│   └── basic.zig          # Usage examples
├── docs/
│   ├── architecture.md    # Architecture overview
│   ├── developer_guide.md # This file
│   └── user_guide.md      # End-user guide
└── README.md              # Quick reference
```

## How to Add a New Binding Function

### Step 1: Add C declaration to `zig_api.h`

```c
void* benchmark_zig_benchmark_new_method(void* benchmark, int param);
```

### Step 2: Implement in `zig_api.cc`

```cpp
void* benchmark_zig_benchmark_new_method(void* b, int param) {
  return static_cast<benchmark::Benchmark*>(b)->NewMethod(param);
}
```

### Step 3: Add Zig wrapper in `benchmark.zig`

```zig
// In Benchmark struct:
pub fn newMethod(self: *Benchmark, param: i32) *Benchmark {
    return .{ .ptr = c.benchmark_zig_benchmark_new_method(self.ptr, param) };
}
```

### Step 4: Add test in `benchmark_test.zig`

```zig
test "new method" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_NewMethod", bm_empty)
        .newMethod(42);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}
```

## Running Tests

```bash
cd bindings/zig
zig build test
```

To run with verbose output:
```bash
zig build test -- --verbose
```

## Building Standalone vs Via CMake

### Standalone (Zig drives the build)

```bash
zig build                           # builds libbenchmark from source
zig build -Dbenchmark_path=/path    # uses pre-built library
```

### Via CMake (CMake drives the build)

```bash
cd /path/to/google-benchmark
cmake -S . -B build -DBENCHMARK_ENABLE_ZIG_BINDINGS=ON
cmake --build build
cd build && ctest -R zig_bindings_tests
```

## CI Integration

The GitHub Actions workflow in `.github/workflows/test_bindings.yml` runs:

1. `zig build test` — native Zig test execution
2. CMake + ctest — integration with the project's test infrastructure

## Code Style

- Zig code follows standard `zig fmt` formatting
- C adapter functions are prefixed with `benchmark_zig_` to avoid symbol collisions
- Zig types use CamelCase for public types (`State`, `Benchmark`, `TimeUnit`)
- Zig functions use camelCase (`keepRunning`, `setBytesProcessed`)

## Debugging

### Building with debug info

```bash
zig build -Doptimize=Debug
```

### Memory debugging

The C++ adapter links against libbenchmark which may allocate. Use AddressSanitizer:

```bash
zig build -Doptimize=Debug -Dsanitize=address
```

### Valgrind

```bash
valgrind ./zig-cache/bin/benchmark_test
```

### Tracing FFI calls

Add `std.log.debug` calls in `benchmark.zig` methods to trace calls across the FFI boundary.
