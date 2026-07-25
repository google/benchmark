// Idiomatic Zig API for google-benchmark.
//
// This module provides a safe, comptime-checked interface to the C++ google-benchmark
// library via the C adapter layer (zig_api.h/cc).
//
// Key design decisions:
//   - State and Benchmark are opaque pointers wrapped in typed Zig structs.
//   - Benchmark configuration methods return self for fluent chaining.
//   - registerBenchmark uses comptime to generate a unique trampoline per function,
//     avoiding heap allocation and runtime dispatch.
//   - Strings use [*:0]const u8 (sentinel-terminated) for C interop safety.

const std = @import("std");

// Import the C adapter functions via @cImport.
const c = @cImport({
    @cInclude("zig_api.h");
});

/// Time unit for benchmark display output.
pub const TimeUnit = enum(c_int) {
    nanosecond = 0,
    microsecond = 1,
    millisecond = 2,
    second = 3,
};

/// Complexity mode for big-O analysis benchmarks.
pub const BigO = enum(c_int) {
    auto = -1,
    o_n = 0,
    o_n_log_n = 1,
    o_1 = 2,
    o_n2 = 3,
};

/// Opaque wrapper around benchmark::State. Passed to benchmark callbacks.
/// Provides methods for iteration control and metric reporting.
pub const State = struct {
    ptr: *anyopaque,

    /// Returns true if the benchmark should continue running.
    /// Call in a while loop: `while (state.keepRunning()) { ... }`
    pub fn keepRunning(self: State) bool {
        return c.benchmark_zig_state_keep_running(self.ptr);
    }

    /// Run n iterations as a batch (more efficient than keepRunning in a loop).
    pub fn keepRunningBatch(self: State, n: i64) bool {
        return c.benchmark_zig_state_keep_running_batch(self.ptr, n);
    }

    /// Pause the benchmark timer (for expensive setup not to be timed).
    pub fn pauseTiming(self: State) void {
        c.benchmark_zig_state_pause_timing(self.ptr);
    }

    /// Resume the benchmark timer after pauseTiming().
    pub fn resumeTiming(self: State) void {
        c.benchmark_zig_state_resume_timing(self.ptr);
    }

    /// Skip this benchmark with an error message.
    pub fn skipWithError(self: State, msg: [*:0]const u8) void {
        c.benchmark_zig_state_skip_with_error(self.ptr, msg);
    }

    /// Report bytes processed per iteration (for throughput metrics).
    pub fn setBytesProcessed(self: State, bytes: i64) void {
        c.benchmark_zig_state_set_bytes_processed(self.ptr, bytes);
    }

    /// Report items processed per iteration (for throughput metrics).
    pub fn setItemsProcessed(self: State, items: i64) void {
        c.benchmark_zig_state_set_items_processed(self.ptr, items);
    }

    /// Set a label for this benchmark run.
    pub fn setLabel(self: State, label: [*:0]const u8) void {
        c.benchmark_zig_state_set_label(self.ptr, label);
    }

    /// Set the complexity parameter N for big-O analysis.
    pub fn setComplexityN(self: State, n: i64) void {
        c.benchmark_zig_state_set_complexity_n(self.ptr, n);
    }

    /// Get the range argument at the given position (0-indexed).
    pub fn range(self: State, pos: usize) i64 {
        return c.benchmark_zig_state_range(self.ptr, pos);
    }

    /// Get the number of iterations completed so far.
    pub fn iterations(self: State) i64 {
        return c.benchmark_zig_state_iterations(self.ptr);
    }

    /// Get the total number of threads.
    pub fn threads(self: State) i32 {
        return c.benchmark_zig_state_threads(self.ptr);
    }

    /// Get the current thread index (0-based).
    pub fn threadIndex(self: State) i32 {
        return c.benchmark_zig_state_thread_index(self.ptr);
    }
};

/// Opaque wrapper around benchmark::Benchmark. Used to configure a benchmark
/// after registration. All configuration methods return self for fluent chaining.
pub const Benchmark = struct {
    ptr: *anyopaque,

    /// Add a single argument value.
    pub fn arg(self: Benchmark, x: i64) Benchmark {
        c.benchmark_zig_benchmark_arg(self.ptr, x);
        return self;
    }

    /// Add a range of arguments from start to limit (doubles each step).
    pub fn range(self: Benchmark, start: i64, limit: i64) Benchmark {
        c.benchmark_zig_benchmark_range(self.ptr, start, limit);
        return self;
    }

    /// Add a dense range of arguments from start to limit with given step.
    pub fn denseRange(self: Benchmark, start: i64, limit: i64, step: i32) Benchmark {
        c.benchmark_zig_benchmark_dense_range(self.ptr, start, limit, step);
        return self;
    }

    /// Add explicit argument values from a slice.
    pub fn args(self: Benchmark, arg_list: []const i64) Benchmark {
        c.benchmark_zig_benchmark_args(self.ptr, arg_list.ptr, arg_list.len);
        return self;
    }

    /// Set the time unit for display.
    pub fn unit(self: Benchmark, u: TimeUnit) Benchmark {
        c.benchmark_zig_benchmark_unit(self.ptr, @intFromEnum(u));
        return self;
    }

    /// Set the number of threads for this benchmark.
    pub fn threads(self: Benchmark, t: i32) Benchmark {
        c.benchmark_zig_benchmark_threads(self.ptr, t);
        return self;
    }

    /// Run with thread count from min_threads to max_threads.
    pub fn threadRange(self: Benchmark, min_threads: i32, max_threads: i32) Benchmark {
        c.benchmark_zig_benchmark_thread_range(self.ptr, min_threads, max_threads);
        return self;
    }

    /// Set minimum run time in seconds.
    pub fn minTime(self: Benchmark, t: f64) Benchmark {
        c.benchmark_zig_benchmark_min_time(self.ptr, t);
        return self;
    }

    /// Set exact iteration count (disables automatic selection).
    pub fn iterations(self: Benchmark, n: i64) Benchmark {
        c.benchmark_zig_benchmark_iterations(self.ptr, n);
        return self;
    }

    /// Set number of repetitions.
    pub fn repetitions(self: Benchmark, n: i32) Benchmark {
        c.benchmark_zig_benchmark_repetitions(self.ptr, n);
        return self;
    }

    /// Use wall-clock time instead of CPU time.
    pub fn useRealTime(self: Benchmark) Benchmark {
        c.benchmark_zig_benchmark_use_real_time(self.ptr);
        return self;
    }

    /// Use manual time control (call SetIterationTime manually).
    pub fn useManualTime(self: Benchmark) Benchmark {
        c.benchmark_zig_benchmark_use_manual_time(self.ptr);
        return self;
    }

    /// Set complexity mode for big-O analysis.
    pub fn complexity(self: Benchmark, b: BigO) Benchmark {
        c.benchmark_zig_benchmark_complexity(self.ptr, @intFromEnum(b));
        return self;
    }

    /// Get the benchmark name as a null-terminated string.
    pub fn getName(self: Benchmark) [*:0]const u8 {
        return c.benchmark_zig_benchmark_name(self.ptr) orelse unreachable;
    }
};

/// Initialize the benchmark library with command-line arguments.
/// Must be called before registerBenchmark() and run().
pub fn initialize(args: []const [*:0]const u8) void {
    var argc: c_int = @intCast(args.len);
    var argv_buf: [64][*c]u8 = undefined;
    const argc_usize: usize = @intCast(args.len);
    const limit = @min(argc_usize, argv_buf.len);
    for (0..limit) |i| {
        argv_buf[i] = @constCast(@ptrCast(args[i]));
    }
    argv_buf[limit] = null;
    c.benchmark_zig_initialize(&argc, &argv_buf);
}

/// Run all registered benchmarks. Returns the number of benchmarks executed.
pub fn run() usize {
    return c.benchmark_zig_run();
}

/// Register a benchmark with the given name and callback function.
///
/// Uses comptime to generate a unique trampoline for each function pointer,
/// avoiding heap allocation. The trampoline bridges the C callback to the
/// Zig function by wrapping the opaque State pointer in a typed struct.
///
/// Returns a Benchmark struct for fluent configuration chaining:
///   benchmark.registerBenchmark("BM_Foo", my_func).range(8, 1 << 20).threads(4);
pub fn registerBenchmark(name: [*:0]const u8, comptime func: *const fn (*State) void) Benchmark {
    // Generate a unique trampoline type at comptime for each benchmark function.
    const S = struct {
        fn trampoline(state_ptr: ?*anyopaque) callconv(.c) void {
            if (state_ptr) |ptr| {
                var state = State{ .ptr = ptr };
                func(&state);
            }
        }
    };
    const result = c.benchmark_zig_register_benchmark(name, &S.trampoline);
    return Benchmark{ .ptr = result orelse unreachable };
}

/// Add a custom key-value context to benchmark output (e.g., for JSON reports).
pub fn addCustomContext(key: [*:0]const u8, value: [*:0]const u8) void {
    c.benchmark_zig_add_custom_context(key, value);
}

/// Remove all registered benchmarks.
pub fn clearRegisteredBenchmarks() void {
    c.benchmark_zig_clear_registered_benchmarks();
}

/// Default main entry point: parses args, initializes, and runs benchmarks.
/// Use this when benchmark is the sole purpose of the program.
pub fn main() void {
    const args = std.process.argsAlloc(std.heap.page_allocator) catch return;
    defer std.process.argsFree(std.heap.page_allocator, args);
    initialize(args);
    _ = run();
}
