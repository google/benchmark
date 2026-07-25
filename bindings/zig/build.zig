const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const benchmark_path = b.option([]const u8, "benchmark_path", "Path to pre-built combined archive") orelse "";

    // ---- Build via CMake (g++ compiles ALL C++ — produces combined .a) ----
    const cmake_step = b.step("cmake", "Build combined archive via CMake");
    var lib_path: []const u8 = undefined;
    if (benchmark_path.len > 0) {
        lib_path = benchmark_path;
    } else {
        const cmake_configure = b.addSystemCommand(&.{
            "cmake", "-S", ".", "-B", "cmake-build",
            "-DBENCHMARK_ENABLE_TESTING=OFF",
            "-DBENCHMARK_ENABLE_LTO=OFF",
            "-DBENCHMARK_ENABLE_WERROR=OFF",
            "-DCMAKE_BUILD_TYPE=Release",
        });
        const cmake_build = b.addSystemCommand(&.{
            "cmake", "--build", "cmake-build", "--config", "Release", "--parallel",
        });
        cmake_build.step.dependOn(&cmake_configure.step);
        cmake_step.dependOn(&cmake_build.step);
        lib_path = "cmake-build/libbenchmark_combined.a";
    }

    // ---- Zig module ----
    const benchmark_module = b.addModule("benchmark", .{
        .root_source_file = b.path("src/benchmark.zig"),
    });
    benchmark_module.addIncludePath(b.path("src"));

    // ---- Tests ----
    const tests = b.addTest(.{
        .root_source_file = b.path("src/benchmark_test.zig"),
        .target = target,
        .optimize = optimize,
    });
    tests.root_module.addImport("benchmark", benchmark_module);
    // Link the combined archive with full path
    tests.addObjectFile(.{ .cwd_relative = lib_path });
    // Link system C++ runtime — use full path to static libstdc++
    tests.addObjectFile(.{ .cwd_relative = "/usr/lib/gcc/x86_64-linux-gnu/14/libstdc++.a" });
    tests.linkSystemLibrary("m");
    tests.linkSystemLibrary("pthread");
    tests.linkSystemLibrary("gcc_s");
    tests.step.dependOn(cmake_step);

    const run_tests = b.addRunArtifact(tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_tests.step);

    const build_step = b.step("build", "Build the library");
    build_step.dependOn(&tests.step);
}
