const std = @import("std");

// ---- Extern declarations (C adapter) ----
const c = @cImport({
    @cInclude("zig_api.h");
});

// ---- Public types ----

pub const TimeUnit = enum(c_int) {
    nanosecond = 0,
    microsecond = 1,
    millisecond = 2,
    second = 3,
};

pub const BigO = enum(c_int) {
    auto = -1,
    o_n = 0,
    o_n_log_n = 1,
    o_1 = 2,
    o_n2 = 3,
};

pub const State = struct {
    ptr: *anyopaque,

    pub fn keepRunning(self: *State) bool {
        return c.benchmark_zig_state_keep_running(self.ptr);
    }

    pub fn keepRunningBatch(self: *State, n: i64) bool {
        return c.benchmark_zig_state_keep_running_batch(self.ptr, n);
    }

    pub fn pauseTiming(self: *State) void {
        c.benchmark_zig_state_pause_timing(self.ptr);
    }

    pub fn resumeTiming(self: *State) void {
        c.benchmark_zig_state_resume_timing(self.ptr);
    }

    pub fn skipWithError(self: *State, msg: [*:0]const u8) void {
        c.benchmark_zig_state_skip_with_error(self.ptr, msg);
    }

    pub fn setBytesProcessed(self: *State, bytes: i64) void {
        c.benchmark_zig_state_set_bytes_processed(self.ptr, bytes);
    }

    pub fn setItemsProcessed(self: *State, items: i64) void {
        c.benchmark_zig_state_set_items_processed(self.ptr, items);
    }

    pub fn setLabel(self: *State, label: [*:0]const u8) void {
        c.benchmark_zig_state_set_label(self.ptr, label);
    }

    pub fn setComplexityN(self: *State, n: i64) void {
        c.benchmark_zig_state_set_complexity_n(self.ptr, n);
    }

    pub fn range(self: *State, pos: usize) i64 {
        return c.benchmark_zig_state_range(self.ptr, pos);
    }

    pub fn iterations(self: *State) i64 {
        return c.benchmark_zig_state_iterations(self.ptr);
    }

    pub fn threads(self: *State) i32 {
        return c.benchmark_zig_state_threads(self.ptr);
    }

    pub fn threadIndex(self: *State) i32 {
        return c.benchmark_zig_state_thread_index(self.ptr);
    }
};

pub const Benchmark = struct {
    ptr: *anyopaque,

    pub fn arg(self: *Benchmark, x: i64) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_arg(self.ptr, x) };
    }

    pub fn range(self: *Benchmark, start: i64, limit: i64) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_range(self.ptr, start, limit) };
    }

    pub fn denseRange(self: *Benchmark, start: i64, limit: i64, step: i32) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_dense_range(self.ptr, start, limit, step) };
    }

    pub fn args(self: *Benchmark, arg_list: []const i64) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_args(self.ptr, arg_list.ptr, arg_list.len) };
    }

    pub fn unit(self: *Benchmark, u: TimeUnit) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_unit(self.ptr, @intFromEnum(u)) };
    }

    pub fn threads(self: *Benchmark, t: i32) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_threads(self.ptr, t) };
    }

    pub fn threadRange(self: *Benchmark, min_threads: i32, max_threads: i32) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_thread_range(self.ptr, min_threads, max_threads) };
    }

    pub fn minTime(self: *Benchmark, t: f64) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_min_time(self.ptr, t) };
    }

    pub fn iterations(self: *Benchmark, n: i64) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_iterations(self.ptr, n) };
    }

    pub fn repetitions(self: *Benchmark, n: i32) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_repetitions(self.ptr, n) };
    }

    pub fn useRealTime(self: *Benchmark) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_use_real_time(self.ptr) };
    }

    pub fn useManualTime(self: *Benchmark) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_use_manual_time(self.ptr) };
    }

    pub fn complexity(self: *Benchmark, b: BigO) *Benchmark {
        return .{ .ptr = c.benchmark_zig_benchmark_complexity(self.ptr, @intFromEnum(b)) };
    }

    pub fn getName(self: *Benchmark) [*:0]const u8 {
        return c.benchmark_zig_benchmark_name(self.ptr);
    }
};

// ---- Public functions ----

pub fn initialize(args: []const [*:0]const u8) void {
    var argc: c_int = @intCast(args.len);
    var argv_ptrs: [][*c]u8 = undefined;
    argv_ptrs.len = args.len;
    argv_ptrs.ptr = @constCast(@ptrCast(args.ptr));
    c.benchmark_zig_initialize(&argc, argv_ptrs.ptr);
}

pub fn run() usize {
    return c.benchmark_zig_run();
}

pub fn registerBenchmark(name: [*:0]const u8, comptime func: *const fn (*State) void) *Benchmark {
    const S = struct {
        fn trampoline(state_ptr: ?*anyopaque) callconv(.c) void {
            if (state_ptr) |ptr| {
                var state = State{ .ptr = ptr };
                func(&state);
            }
        }
    };
    return .{ .ptr = c.benchmark_zig_register_benchmark(name.ptr, &S.trampoline) };
}

pub fn addCustomContext(key: [*:0]const u8, value: [*:0]const u8) void {
    c.benchmark_zig_add_custom_context(key.ptr, value.ptr);
}

pub fn clearRegisteredBenchmarks() void {
    c.benchmark_zig_clear_registered_benchmarks();
}

pub fn main() void {
    const args = std.process.argsAlloc(std.heap.page_allocator) catch return;
    defer std.process.argsFree(std.heap.page_allocator, args);
    initialize(args);
    _ = run();
}
