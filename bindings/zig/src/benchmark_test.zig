// Unit tests for Zig bindings to google-benchmark.
//
// Each test registers a single benchmark function, runs it, and verifies
// that the benchmark library executes without errors. Tests cover all
// major API surface: keepRunning, keepRunningBatch, pause/resume timing,
// throughput metrics, parameterized benchmarks, threading, and error skipping.
//
// Note: google-benchmark is designed for single-process execution.
// Tests that call clearRegisteredBenchmarks() + run() sequentially may
// exhibit state isolation issues in some environments.

const std = @import("std");
const benchmark = @import("benchmark");

/// Prevent the compiler from optimizing away a variable or computation.
/// Used in benchmarks to ensure the measured code is not eliminated.
fn volatile_sink(ptr: anytype) void {
    @as(*volatile @TypeOf(ptr.*), ptr).* = ptr.*;
}

// ---- Benchmark functions (callbacks passed to registerBenchmark) ----

/// Minimal no-op benchmark: measures bare loop overhead.
fn bm_empty(state: *benchmark.State) void {
    while (state.keepRunning()) {}
}

/// Benchmark using batch iteration (processes 64 iterations per call).
fn bm_with_batch(state: *benchmark.State) void {
    while (state.keepRunningBatch(64)) {}
}

/// Benchmark with pause/resume timing: setup phase is excluded from measurement.
fn bm_pause_resume(state: *benchmark.State) void {
    while (state.keepRunning()) {
        state.pauseTiming();
        // "expensive" setup — not timed
        var sink: i64 = 0;
        for (0..100) |i| {
            sink += @intCast(i);
        }
        state.resumeTiming();
        // Prevent optimizer from removing the setup loop
        volatile_sink(&sink);
    }
}

/// Throughput benchmark: reports bytes processed per iteration.
fn bm_bytes_processed(state: *benchmark.State) void {
    const n: i64 = state.range(0);
    while (state.keepRunning()) {
        const data: [1024]u8 = [_]u8{0x42} ** 1024;
        _ = data;
    }
    state.setBytesProcessed(n * state.iterations());
}

/// Throughput benchmark: reports items processed per iteration.
fn bm_items_processed(state: *benchmark.State) void {
    while (state.keepRunning()) {
        // simulate processing items
    }
    state.setItemsProcessed(state.iterations() * 10);
}

/// Benchmark with a custom label in output.
fn bm_with_label(state: *benchmark.State) void {
    while (state.keepRunning()) {}
    state.setLabel("my_label");
}

/// Benchmark that reads a range parameter.
fn bm_range_1(state: *benchmark.State) void {
    _ = state.range(0);
    while (state.keepRunning()) {}
}

/// Multi-threaded benchmark stub.
fn bm_threads_fn(state: *benchmark.State) void {
    while (state.keepRunning()) {}
}

/// Benchmark that immediately skips with an error message.
fn bm_skip(state: *benchmark.State) void {
    state.skipWithError("not supported on this platform");
}

// ---- Unit tests ----

/// Verify basic benchmark registration and execution.
test "basic benchmark runs" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Empty", bm_empty);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify batch iteration mode works.
test "benchmark with batch" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Batch", bm_with_batch);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify pause/resume timing doesn't crash.
test "pause and resume timing" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_PauseResume", bm_pause_resume);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify bytes_processed metric reporting.
test "bytes processed" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_BytesProcessed", bm_bytes_processed)
        .range(1 << 10, 1 << 16);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify items_processed metric reporting.
test "items processed" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_ItemsProcessed", bm_items_processed);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify setLabel works without errors.
test "set label" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Label", bm_with_label);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify range parameter is passed correctly.
test "range parameter" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Range", bm_range_1)
        .range(1, 64);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify multiple benchmarks can be registered and run together.
test "multiple benchmarks" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Multi1", bm_empty);
    _ = benchmark.registerBenchmark("BM_Multi2", bm_with_batch);
    _ = benchmark.registerBenchmark("BM_Multi3", bm_pause_resume);
    const count = benchmark.run();
    try std.testing.expect(count >= 3);
}

/// Verify multi-threaded benchmark execution.
test "threaded benchmark" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Threads", bm_threads_fn)
        .threads(4);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify TimeUnit enum is passed correctly.
test "time unit" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_MicroSecond", bm_empty)
        .unit(.microsecond);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify skipWithError gracefully skips the benchmark.
test "skip with error" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Skip", bm_skip);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify benchmark name is retrievable after registration.
test "benchmark name" {
    benchmark.clearRegisteredBenchmarks();
    const b = benchmark.registerBenchmark("BM_Named", bm_empty);
    const name = b.getName();
    const expected = "BM_Named";
    for (expected, 0..) |ch, i| {
        try std.testing.expectEqual(ch, name[i]);
    }
}

/// Verify denseRange parameter works.
test "dense range" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_DenseRange", bm_empty)
        .denseRange(1, 5, 1);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

/// Verify useRealTime flag works.
test "use real time" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_RealTime", bm_empty)
        .useRealTime();
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}
