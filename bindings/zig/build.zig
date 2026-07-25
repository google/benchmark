// Build system for Zig bindings to google-benchmark.
//
// Strategy: build libbenchmark + C adapter via CMake (using the system's
// default C++ compiler), producing a combined static archive. Zig links
// against this archive without compiling any C++ itself.
//
// Usage:
//   zig build              — build and run tests
//   zig build test         — run unit tests only
//   zig build cmake        — rebuild the combined archive only
//   zig build -Dbenchmark_path=/path/to/lib.a  — use pre-built archive

const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Optional: path to a pre-built combined archive (skips cmake build).
    const benchmark_path = b.option([]const u8, "benchmark_path", "Path to pre-built combined archive") orelse "";

    // ---- CMake step: builds libbenchmark + adapter into combined .a ----
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

    // ---- Zig module: public API for users to depend on ----
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
    // Link the combined archive containing all C++ symbols
    tests.addObjectFile(.{ .cwd_relative = lib_path });
    // Link the C++ runtime matching the compiler used to build libbenchmark.
    // cmake uses the system default compiler (g++ on Linux, clang++ on macOS).
    // We detect the platform and link accordingly.
    const os_tag = target.result.os.tag;
    if (os_tag == .macos) {
        // macOS: system default is clang++ which uses libc++
        tests.linkLibCpp();
        tests.linkFramework("System");
    } else if (os_tag == .linux) {
        // Linux: system default is g++ which uses libstdc++
        // Find the system libstdc++ by running g++ to get its library path
        tests.addLibraryPath(.{ .cwd_relative = "/usr/lib/gcc/x86_64-linux-gnu/14" });
        tests.linkSystemLibrary("stdc++");
        tests.linkSystemLibrary("pthread");
        tests.linkSystemLibrary("m");
    } else {
        // Fallback for other platforms
        tests.linkLibCpp();
        tests.linkSystemLibrary("m");
        tests.linkSystemLibrary("pthread");
    }
    tests.step.dependOn(cmake_step);

    const run_tests = b.addRunArtifact(tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_tests.step);

    const build_step = b.step("build", "Build the library");
    build_step.dependOn(&tests.step);
}
