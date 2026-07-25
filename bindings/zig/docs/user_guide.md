# User Guide

## Installation

### As a Zig dependency

Add to your `build.zig.zon`:

```zig
.{
    .dependencies = .{
        .benchmark = .{
            .url = "https://github.com/google/benchmark/archive/refs/tags/v1.9.5.tar.gz",
            .hash = "...",  // run zig build to get the hash
        },
    },
}
```

Then in your `build.zig`:

```zig
const benchmark_dep = b.dependency("benchmark", .{});
const benchmark_module = benchmark_dep.module("benchmark");
exe.root_module.addImport("benchmark", benchmark_module);
```

### System-wide

Build and install libbenchmark first, then use `-Dbenchmark_path`:

```bash
cd bindings/zig
zig build -Dbenchmark_path=/usr/local/lib
```

## First Benchmark

```zig
const std = @import("std");
const benchmark = @import("benchmark");

fn BM_hello(state: *benchmark.State) void {
    while (state.keepRunning()) {
        // Your code here
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

Run it:
```bash
zig run my_benchmark.zig
```

Output:
```
Running BM_hello
Run on (8 X 3600 MHz CPU s)
Load Average: 0.50, 0.30, 0.10
----------------------------------------------------------------------
Benchmark            Time             CPU   Iterations
----------------------------------------------------------------------
BM_hello           123 ns          123 ns      5678901
```

## Common Patterns

### Throughput Measurement

```zig
fn BM_throughput(state: *benchmark.State) void {
    const n: usize = @intCast(state.range(0));
    const data = allocator.alloc(u8, n) catch return;
    defer allocator.free(data);

    while (state.keepRunning()) {
        @memset(data, 0x42);
    }
    state.setBytesProcessed(@intCast(n * @as(usize, @intCast(state.iterations()))));
}

// Register with:
_ = benchmark.registerBenchmark("BM_throughput", BM_throughput)
    .range(1 << 10, 1 << 20)
    .unit(.kilobyte);
```

### Parameterized Benchmarks

```zig
fn BM_parameterized(state: *benchmark.State) void {
    const n: usize = @intCast(state.range(0));
    // n varies: 1, 2, 4, 8, 16, ...
    while (state.keepRunning()) {
        // work with size n
    }
}

// Register with:
_ = benchmark.registerBenchmark("BM_parameterized", BM_parameterized)
    .range(1, 1 << 16);
```

### Multi-argument Benchmarks

```zig
fn BM_multi_arg(state: *benchmark.State) void {
    const rows = state.range(0);
    const cols = state.range(1);
    // work with rows x cols
    while (state.keepRunning()) {}
}

// Register with:
_ = benchmark.registerBenchmark("BM_multi_arg", BM_multi_arg)
    .args(&.{ 64, 64 })
    .args(&.{ 128, 128 })
    .args(&.{ 256, 256 });
```

### Pause/Resume Timing

```zig
fn BM_with_setup(state: *benchmark.State) void {
    while (state.keepRunning()) {
        state.pauseTiming();
        // Expensive setup (not timed)
        setup_data();
        state.resumeTiming();
        // Actual work (timed)
        do_work();
    }
}
```

### Multi-threaded Benchmarks

```zig
fn BM_threaded(state: *benchmark.State) void {
    while (state.keepRunning()) {
        // Parallelizable work
    }
}

// Register with:
_ = benchmark.registerBenchmark("BM_threaded", BM_threaded)
    .threads(1)
    .threads(2)
    .threads(4)
    .threads(8);
```

### Custom Counters

```zig
fn BM_custom_counter(state: *benchmark.State) void {
    while (state.keepRunning()) {}
    state.setItemsProcessed(state.iterations() * 100);
    state.setLabel("custom_label");
}
```

### Skip Benchmarks

```zig
fn BM_conditional(state: *benchmark.State) void {
    if (comptime !feature_is_available()) {
        state.skipWithError("feature not available");
        return;
    }
    while (state.keepRunning()) {}
}
```

## Command-Line Flags

Pass flags through `initialize`:

```zig
const args = [_][*:0]const u8{
    "benchmark",
    "--benchmark_format=console",
    "--benchmark_min_time=0.5",
    "--benchmark_repetitions=3",
    "--benchmark_filter=BM_hello",
};
```

Common flags:

| Flag | Description |
|---|---|
| `--benchmark_format=console` | Output format (console, json, csv) |
| `--benchmark_min_time=0.5` | Minimum time per benchmark (seconds) |
| `--benchmark_repetitions=3` | Number of repetitions |
| `--benchmark_filter=BM_.*` | Regex filter for benchmark names |
| `--benchmark_list_tests` | List all benchmarks without running |
| `--benchmark_report_aggregates_only=true` | Only show aggregates |

## Interpreting Output

```
BM_sort/8           123 ns          121 ns      5678901
│     │              │                │            │
│     │              │                │            └─ iterations run
│     │              │                └─ CPU time per iteration
│     │              └─ wall-clock time per iteration
│     └─ argument (range(0))
└─ benchmark name
```

- **Time**: Lower is better (unless measuring throughput)
- **CPU vs Time**: If using `useRealTime()`, Time shows wall-clock; otherwise both show CPU time
- **Iterations**: More iterations = more statistical confidence
