fn main() {
    let mut config = cmake::Config::new("../../");
    config
        .define("BENCHMARK_ENABLE_TESTING", "OFF")
        .define("BENCHMARK_ENABLE_LTO", "OFF")
        .define("BENCHMARK_ENABLE_WERROR", "OFF")
        .build_target("benchmark");

    // Rust defaults to the Release CRT (/MD) on Windows even in Debug mode.
    // Force CMake to use the Release profile so `google/benchmark` uses `/MD`,
    // avoiding a mismatch with `cxx_build` (which uses `/MD`).
    if cfg!(target_os = "windows") {
        config.profile("Release");
    }

    let dst = config.build();

    println!("cargo:rustc-link-search=native={}/build/src", dst.display());
    println!("cargo:rustc-link-search=native={}/build/src/Debug", dst.display());
    println!("cargo:rustc-link-search=native={}/build/src/Release", dst.display());
    println!("cargo:rustc-link-lib=static=benchmark");

    cxx_build::bridge("src/ffi.rs")
        .file("src/rust_api.cc")
        .include("../../include")
        .include("src")
        .std("c++17")
        .define("BENCHMARK_STATIC_DEFINE", None)
        .compile("benchmark_rust_ffi");

    if cfg!(target_os = "windows") {
        println!("cargo:rustc-link-lib=shlwapi");
    }

    println!("cargo:rerun-if-changed=src/ffi.rs");
    println!("cargo:rerun-if-changed=src/rust_api.cc");
    println!("cargo:rerun-if-changed=src/rust_api.h");
    println!("cargo:rerun-if-changed=../../src/");
    println!("cargo:rerun-if-changed=../../include/");
}
