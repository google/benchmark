const std = @import("std");
const benchmark = @import("benchmark");

fn volatile_sink(ptr: anytype) void {
    @as(*volatile @TypeOf(ptr.*), ptr).* = ptr.*;
}

// ---- Example 1: Basic benchmark ----

fn BM_string_creation(state: *benchmark.State) void {
    while (state.keepRunning()) {
        const s = "Hello, World!";
        volatile_sink(&s.ptr);
    }
}

// ---- Example 2: Throughput benchmark ----

fn BM_memory_write(state: *benchmark.State) void {
    const n: usize = @intCast(state.range(0));
    const data = std.heap.page_allocator.alloc(u8, n) catch return;
    defer std.heap.page_allocator.free(data);

    while (state.keepRunning()) {
        @memset(data, 0x42);
    }
    state.setBytesProcessed(@intCast(n * @as(usize, @intCast(state.iterations()))));
}

// ---- Example 3: Throughput with items ----

fn BM_vector_push_back(state: *benchmark.State) void {
    while (state.keepRunning()) {
        var vec = std.ArrayList(u32).init(std.heap.page_allocator);
        defer vec.deinit();
        for (0..1000) |i| {
            vec.append(@intCast(i)) catch break;
        }
    }
    state.setItemsProcessed(state.iterations() * 1000);
}

// ---- Example 4: Parameterized benchmark ----

fn BM_sort_merge(state: *benchmark.State) void {
    const n: usize = @intCast(state.range(0));
    var rng = std.Random.DefaultPrng.init(42);
    const allocator = std.heap.page_allocator;

    var arr = allocator.alloc(i32, n) catch return;
    defer allocator.free(arr);

    for (arr) |*item| {
        item.* = rng.random().int(i32);
    }

    while (state.keepRunning()) {
        std.mem.sort(i32, arr, {}, std.sort.asc(i32));
    }
}

// ---- Example 5: Pause/resume timing ----

fn BM_with_setup(state: *benchmark.State) void {
    while (state.keepRunning()) {
        state.pauseTiming();
        // Expensive setup that should not be timed
        var sink: i64 = 0;
        for (0..1000) |i| {
            sink +%= @intCast(i);
        }
        state.resumeTiming();
        volatile_sink(&sink);
    }
}

// ---- Example 6: Threaded benchmark ----

fn BM_threaded(state: *benchmark.State) void {
    while (state.keepRunning()) {
        // Work that benefits from parallelism
        var sum: i64 = 0;
        for (0..10000) |i| {
            sum +%= @intCast(i);
        }
        volatile_sink(&sum);
    }
}

// ---- Example 7: Skip benchmark ----

fn BM_platform_specific(state: *benchmark.State) void {
    if (comptime !std.Target.current.os.tag.isLinux()) {
        state.skipWithError("only supported on Linux");
        return;
    }
    while (state.keepRunning()) {}
}

// ---- Main ----

pub fn main() void {
    const args = std.process.argsAlloc(std.heap.page_allocator) catch return;
    defer std.process.argsFree(std.heap.page_allocator, args);

    benchmark.initialize(args);

    _ = benchmark.registerBenchmark("BM_string_creation", BM_string_creation);

    _ = benchmark.registerBenchmark("BM_memory_write", BM_memory_write)
        .range(1 << 10, 1 << 20);

    _ = benchmark.registerBenchmark("BM_vector_push_back", BM_vector_push_back);

    _ = benchmark.registerBenchmark("BM_sort_merge", BM_sort_merge)
        .range(1 << 0, 1 << 12)
        .unit(.microsecond);

    _ = benchmark.registerBenchmark("BM_with_setup", BM_with_setup);

    _ = benchmark.registerBenchmark("BM_threaded", BM_threaded)
        .threads(1)
        .threads(2)
        .threads(4);

    _ = benchmark.registerBenchmark("BM_platform_specific", BM_platform_specific);

    _ = benchmark.run();
}
