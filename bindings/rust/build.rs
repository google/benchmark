use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap());
    let repo_root = manifest_dir
        .join("../..")
        .canonicalize()
        .expect("could not resolve repo root");

    // Build the benchmark C++ static library via CMake.
    // C++ maintainers who do not run `cargo` are unaffected; CMake only
    // enters this path when Rust users invoke `cargo build`.
    let dst = cmake::Config::new(&repo_root)
        .define("BENCHMARK_ENABLE_TESTING", "OFF")
        .define("BENCHMARK_ENABLE_GTEST_TESTS", "OFF")
        .define("BUILD_SHARED_LIBS", "OFF")
        .define("BENCHMARK_ENABLE_INSTALL", "OFF")
        .build_target("benchmark")
        .build();

    // cmake crate places build artifacts under <OUT_DIR>/build/
    let cmake_build = dst.join("build");

    // Compile the cxx bridge together with our thin C++ wrapper.
    cxx_build::bridge("src/lib.rs")
        .file("src/bridge.cc")
        .include(repo_root.join("include"))
        .include(manifest_dir.join("src")) // for bridge.h
        .flag_if_supported("-std=c++14")
        .compile("benchmark-bridge");

    // Link against the static benchmark library built above.
    // The CMake sub-build puts the library in src/ inside the build tree.
    println!(
        "cargo:rustc-link-search=native={}",
        cmake_build.join("src").display()
    );
    // Also cover multi-config generators (e.g. Xcode, MSVC).
    for config in &["Release", "Debug", "RelWithDebInfo"] {
        println!(
            "cargo:rustc-link-search=native={}",
            cmake_build.join("src").join(config).display()
        );
    }
    println!("cargo:rustc-link-lib=static=benchmark");

    // Link the C++ standard library.
    let target = std::env::var("TARGET").unwrap_or_default();
    if target.contains("apple") {
        println!("cargo:rustc-link-lib=c++");
    } else if target.contains("windows") {
        // MSVC links automatically; MinGW needs stdc++
        if target.contains("gnu") {
            println!("cargo:rustc-link-lib=stdc++");
        }
    } else {
        println!("cargo:rustc-link-lib=stdc++");
    }

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge.cc");
    println!("cargo:rerun-if-changed=src/bridge.h");
}
