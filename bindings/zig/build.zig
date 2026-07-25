const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Option: path to pre-built benchmark library
    const benchmark_path = b.option([]const u8, "benchmark_path", "Path to pre-built libbenchmark directory") orelse "";

    // ---- Build libbenchmark via CMake if no path given ----
    const lib_dir = if (benchmark_path.len > 0)
        benchmark_path
    else
        buildBenchmarkFromSource(b, target, optimize);

    // ---- Build the C++ adapter ----
    const adapter = b.addStaticLibrary(.{
        .name = "zig_api",
        .target = target,
        .optimize = optimize,
    });
    adapter.addIncludePath(b.path("src"));
    adapter.addIncludePath(b.path("../../include"));
    adapter.addCSourceFile(.{
        .file = b.path("src/zig_api.cc"),
        .flags = &.{"-std=c++17"},
    });
    adapter.linkLibCpp();

    // Link against libbenchmark
    adapter.addLibraryPath(.{ .cwd_relative = lib_dir });
    adapter.linkSystemLibrary("benchmark");

    b.installArtifact(adapter);

    // ---- Zig module ----
    const benchmark_module = b.addModule("benchmark", .{
        .root_source_file = b.path("src/benchmark.zig"),
    });
    benchmark_module.addIncludePath(b.path("src"));
    benchmark_module.linkLibrary(adapter);

    // ---- Tests ----
    const tests = b.addTest(.{
        .root_source_file = b.path("src/benchmark_test.zig"),
        .target = target,
        .optimize = optimize,
    });
    tests.root_module.addImport("benchmark", benchmark_module);
    tests.linkLibrary(adapter);

    const run_tests = b.addRunArtifact(tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_tests.step);

    // ---- Build step ----
    const build_step = b.step("build", "Build the library");
    build_step.dependOn(&adapter.step);
}

fn buildBenchmarkFromSource(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) []const u8 {
    _ = target;
    _ = optimize;

    // Run cmake configure
    const cmake_configure = b.addSystemCommand(&.{
        "cmake",
        "-S",
        "../../",
        "-B",
        "cmake-build",
        "-DBENCHMARK_ENABLE_TESTING=OFF",
        "-DBENCHMARK_ENABLE_LTO=OFF",
        "-DBENCHMARK_ENABLE_WERROR=OFF",
        "-DBENCHMARK_ENABLE_INSTALL=OFF",
        "-DCMAKE_BUILD_TYPE=Release",
    });
    b.getInstallStep().dependOn(&cmake_configure.step);

    // Run cmake build
    const cmake_build = b.addSystemCommand(&.{
        "cmake",
        "--build",
        "cmake-build",
        "--config",
        "Release",
        "--parallel",
    });
    cmake_build.step.dependOn(&cmake_configure.step);
    b.getInstallStep().dependOn(&cmake_build.step);

    return "cmake-build/src";
}
