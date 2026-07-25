const std = @import("std");
const benchmark = @import("benchmark");

// ---- Benchmark functions ----

fn bm_empty(state: *benchmark.State) void {
    while (state.keepRunning()) {}
}

fn bm_with_batch(state: *benchmark.State) void {
    while (state.keepRunningBatch(64)) {}
}

fn bm_pause_resume(state: *benchmark.State) void {
    while (state.keepRunning()) {
        state.pauseTiming();
        // "expensive" setup
        var sum: i64 = 0;
        for (0..100) |i| {
            sum += @intCast(i);
        }
        state.resumeTiming();
        _ = sum;
    }
}

fn bm_bytes_processed(state: *benchmark.State) void {
    const n: usize = @intCast(state.range(0));
    while (state.keepRunning()) {
        const data: [1024]u8 = [_]u8{0x42} ** 1024;
        _ = data;
    }
    state.setBytesProcessed(n * state.iterations());
}

fn bm_items_processed(state: *benchmark.State) void {
    while (state.keepRunning()) {
        // simulate processing items
    }
    state.setItemsProcessed(state.iterations() * 10);
}

fn bm_with_label(state: *benchmark.State) void {
    while (state.keepRunning()) {}
    state.setLabel("my_label");
}

fn bm_range_1(state: *benchmark.State) void {
    _ = state.range(0);
    while (state.keepRunning()) {}
}

fn bm_threads_fn(state: *benchmark.State) void {
    while (state.keepRunning()) {}
}

fn bm_skip(state: *benchmark.State) void {
    state.skipWithError("not supported on this platform");
}

// ---- Tests ----

test "basic benchmark runs" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Empty", bm_empty);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "benchmark with batch" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Batch", bm_with_batch);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "pause and resume timing" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_PauseResume", bm_pause_resume);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "bytes processed" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_BytesProcessed", bm_bytes_processed)
        .range(1 << 10, 1 << 16);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "items processed" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_ItemsProcessed", bm_items_processed);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "set label" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Label", bm_with_label);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "range parameter" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Range", bm_range_1)
        .range(1, 64);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "multiple benchmarks" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Multi1", bm_empty);
    _ = benchmark.registerBenchmark("BM_Multi2", bm_with_batch);
    _ = benchmark.registerBenchmark("BM_Multi3", bm_pause_resume);
    const count = benchmark.run();
    try std.testing.expect(count >= 3);
}

test "threaded benchmark" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Threads", bm_threads_fn)
        .threads(4);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "time unit" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_MicroSecond", bm_empty)
        .unit(.microsecond);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "skip with error" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_Skip", bm_skip);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "benchmark name" {
    benchmark.clearRegisteredBenchmarks();
    const b = benchmark.registerBenchmark("BM_Named", bm_empty);
    const name = b.getName();
    const expected = "BM_Named";
    for (expected, 0..) |ch, i| {
        try std.testing.expectEqual(ch, name[i]);
    }
}

test "dense range" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_DenseRange", bm_empty)
        .denseRange(1, 5, 1);
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}

test "use real time" {
    benchmark.clearRegisteredBenchmarks();
    _ = benchmark.registerBenchmark("BM_RealTime", bm_empty)
        .useRealTime();
    const count = benchmark.run();
    try std.testing.expect(count > 0);
}
