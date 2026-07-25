const std = @import("std");

const c = @cImport({
    @cInclude("zig_api.h");
});

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

    pub fn keepRunning(self: State) bool {
        return c.benchmark_zig_state_keep_running(self.ptr);
    }

    pub fn keepRunningBatch(self: State, n: i64) bool {
        return c.benchmark_zig_state_keep_running_batch(self.ptr, n);
    }

    pub fn pauseTiming(self: State) void {
        c.benchmark_zig_state_pause_timing(self.ptr);
    }

    pub fn resumeTiming(self: State) void {
        c.benchmark_zig_state_resume_timing(self.ptr);
    }

    pub fn skipWithError(self: State, msg: [*:0]const u8) void {
        c.benchmark_zig_state_skip_with_error(self.ptr, msg);
    }

    pub fn setBytesProcessed(self: State, bytes: i64) void {
        c.benchmark_zig_state_set_bytes_processed(self.ptr, bytes);
    }

    pub fn setItemsProcessed(self: State, items: i64) void {
        c.benchmark_zig_state_set_items_processed(self.ptr, items);
    }

    pub fn setLabel(self: State, label: [*:0]const u8) void {
        c.benchmark_zig_state_set_label(self.ptr, label);
    }

    pub fn setComplexityN(self: State, n: i64) void {
        c.benchmark_zig_state_set_complexity_n(self.ptr, n);
    }

    pub fn range(self: State, pos: usize) i64 {
        return c.benchmark_zig_state_range(self.ptr, pos);
    }

    pub fn iterations(self: State) i64 {
        return c.benchmark_zig_state_iterations(self.ptr);
    }

    pub fn threads(self: State) i32 {
        return c.benchmark_zig_state_threads(self.ptr);
    }

    pub fn threadIndex(self: State) i32 {
        return c.benchmark_zig_state_thread_index(self.ptr);
    }
};

pub const Benchmark = struct {
    ptr: *anyopaque,

    pub fn arg(self: Benchmark, x: i64) Benchmark {
        c.benchmark_zig_benchmark_arg(self.ptr, x);
        return self;
    }

    pub fn range(self: Benchmark, start: i64, limit: i64) Benchmark {
        c.benchmark_zig_benchmark_range(self.ptr, start, limit);
        return self;
    }

    pub fn denseRange(self: Benchmark, start: i64, limit: i64, step: i32) Benchmark {
        c.benchmark_zig_benchmark_dense_range(self.ptr, start, limit, step);
        return self;
    }

    pub fn args(self: Benchmark, arg_list: []const i64) Benchmark {
        c.benchmark_zig_benchmark_args(self.ptr, arg_list.ptr, arg_list.len);
        return self;
    }

    pub fn unit(self: Benchmark, u: TimeUnit) Benchmark {
        c.benchmark_zig_benchmark_unit(self.ptr, @intFromEnum(u));
        return self;
    }

    pub fn threads(self: Benchmark, t: i32) Benchmark {
        c.benchmark_zig_benchmark_threads(self.ptr, t);
        return self;
    }

    pub fn threadRange(self: Benchmark, min_threads: i32, max_threads: i32) Benchmark {
        c.benchmark_zig_benchmark_thread_range(self.ptr, min_threads, max_threads);
        return self;
    }

    pub fn minTime(self: Benchmark, t: f64) Benchmark {
        c.benchmark_zig_benchmark_min_time(self.ptr, t);
        return self;
    }

    pub fn iterations(self: Benchmark, n: i64) Benchmark {
        c.benchmark_zig_benchmark_iterations(self.ptr, n);
        return self;
    }

    pub fn repetitions(self: Benchmark, n: i32) Benchmark {
        c.benchmark_zig_benchmark_repetitions(self.ptr, n);
        return self;
    }

    pub fn useRealTime(self: Benchmark) Benchmark {
        c.benchmark_zig_benchmark_use_real_time(self.ptr);
        return self;
    }

    pub fn useManualTime(self: Benchmark) Benchmark {
        c.benchmark_zig_benchmark_use_manual_time(self.ptr);
        return self;
    }

    pub fn complexity(self: Benchmark, b: BigO) Benchmark {
        c.benchmark_zig_benchmark_complexity(self.ptr, @intFromEnum(b));
        return self;
    }

    pub fn getName(self: Benchmark) [*:0]const u8 {
        return c.benchmark_zig_benchmark_name(self.ptr) orelse unreachable;
    }
};

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

pub fn run() usize {
    return c.benchmark_zig_run();
}

pub fn registerBenchmark(name: [*:0]const u8, comptime func: *const fn (*State) void) Benchmark {
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

pub fn addCustomContext(key: [*:0]const u8, value: [*:0]const u8) void {
    c.benchmark_zig_add_custom_context(key, value);
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
