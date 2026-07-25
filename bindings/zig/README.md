# Google Benchmark Zig Bindings

Zig bindings for the [Google Benchmark](https://github.com/google/benchmark) microbenchmark library.

## Prerequisites

- Zig 0.11.0 or later
- CMake 3.13+
- C++17 compatible compiler (GCC, Clang, or MSVC)

## Quick Start

```zig
const benchmark = @import("benchmark");

fn BM_hello(state: *benchmark.State) void {
    while (state.keepRunning()) {
        // your code to benchmark here
    }
}

pub fn main() void {
    const args = std.process.argsAlloc(std.heap.page_allocator) catch return;
    defer std.process.argsFree(std.heap.page_allocator, args);

    benchmark.initialize(args);
    _ = benchmark.registerBenchmark("BM_hello", BM_hello);
    _ = benchmark.run();
}
```

## Building

### Standalone (builds libbenchmark from source)

```bash
cd bindings/zig
zig build
```

### Against pre-installed libbenchmark

```bash
cd bindings/zig
zig build -Dbenchmark_path=/path/to/lib
```

### Via CMake (from project root)

```bash
cmake -S . -B build -DBENCHMARK_ENABLE_ZIG_BINDINGS=ON
cmake --build build
cd build && ctest -R zig_bindings_tests
```

## Running Tests

```bash
cd bindings/zig
zig build test
```

## Running Examples

```bash
cd bindings/zig
zig run examples/basic.zig
```

## API

### Top-level functions

| Function | Description |
|---|---|
| `initialize(args)` | Initialize the benchmark library |
| `run()` | Run all registered benchmarks, returns count |
| `registerBenchmark(name, func)` | Register a benchmark function, returns `*Benchmark` for chaining |
| `addCustomContext(key, value)` | Add custom context to JSON output |
| `clearRegisteredBenchmarks()` | Remove all registered benchmarks |

### `State`

| Method | Description |
|---|---|
| `keepRunning()` | Returns true if the benchmark should continue |
| `keepRunningBatch(n)` | Returns true, processes `n` iterations at once |
| `pauseTiming()` | Pause the benchmark timer |
| `resumeTiming()` | Resume the benchmark timer |
| `skipWithError(msg)` | Skip this benchmark with an error message |
| `setBytesProcessed(n)` | Report bytes processed per iteration |
| `setItemsProcessed(n)` | Report items processed per iteration |
| `setLabel(str)` | Set a label for this iteration |
| `setComplexityN(n)` | Set the complexity parameter N |
| `range(pos)` | Get the range argument at position `pos` |
| `iterations()` | Get the number of iterations run |
| `threads()` | Get the number of threads |
| `threadIndex()` | Get the current thread index |

### `Benchmark`

All configuration methods return `*Benchmark` for fluent chaining.

| Method | Description |
|---|---|
| `arg(x)` | Add a single argument |
| `range(start, limit)` | Add a range of arguments (doubles each time) |
| `denseRange(start, limit, step)` | Add a dense range (adds each step) |
| `args(list)` | Add a list of arguments |
| `unit(time_unit)` | Set the time unit |
| `threads(n)` | Set the number of threads |
| `threadRange(min, max)` | Run with thread count from min to max |
| `minTime(t)` | Set minimum run time in seconds |
| `iterations(n)` | Set exact iteration count |
| `repetitions(n)` | Set number of repetitions |
| `useRealTime()` | Use wall-clock time instead of CPU time |
| `useManualTime()` | Use manual time control |
| `complexity(bigo)` | Set complexity mode |
| `getName()` | Get the benchmark name |

### Enums

```zig
pub const TimeUnit = enum(c_int) {
    nanosecond, microsecond, millisecond, second,
};

pub const BigO = enum(c_int) {
    auto, o_n, o_n_log_n, o_1, o_n2,
};
```
