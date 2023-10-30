"""
This file contains the Bazel build dependencies for Google Benchmark (both C++ source and Python bindings).
"""

load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def benchmark_deps():
    """Loads dependencies required to build Google Benchmark."""

    if "bazel_skylib" not in native.existing_rules():
        http_archive(
            name = "bazel_skylib",
            sha256 = "66ffd9315665bfaafc96b52278f57c7e2dd09f5ede279ea6d39b2be471e7e3aa",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.4.2/bazel-skylib-1.4.2.tar.gz",
                "https://github.com/bazelbuild/bazel-skylib/releases/download/1.4.2/bazel-skylib-1.4.2.tar.gz",
            ],
        )

    if "rules_foreign_cc" not in native.existing_rules():
        http_archive(
            name = "rules_foreign_cc",
            sha256 = "2a4d07cd64b0719b39a7c12218a3e507672b82a97b98c6a89d38565894cf7c51",
            strip_prefix = "rules_foreign_cc-0.9.0",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.9.0.tar.gz",
        )

    if "rules_python" not in native.existing_rules():
        http_archive(
            name = "rules_python",
            sha256 = "0a8003b044294d7840ac7d9d73eef05d6ceb682d7516781a4ec62eeb34702578",
            url = "https://github.com/bazelbuild/rules_python/releases/download/0.24.0/rules_python-0.24.0.tar.gz",
            strip_prefix = "rules_python-0.24.0",
        )

    if "com_google_absl" not in native.existing_rules():
        http_archive(
            name = "com_google_absl",
            sha256 = "f41868f7a938605c92936230081175d1eae87f6ea2c248f41077c8f88316f111",
            strip_prefix = "abseil-cpp-20200225.2",
            urls = ["https://github.com/abseil/abseil-cpp/archive/20200225.2.tar.gz"],
        )

    if "com_google_googletest" not in native.existing_rules():
        new_git_repository(
            name = "com_google_googletest",
            remote = "https://github.com/google/googletest.git",
            tag = "release-1.11.0",
        )

    if "nanobind" not in native.existing_rules():
        new_git_repository(
            name = "nanobind",
            remote = "https://github.com/wjakob/nanobind.git",
            tag = "v1.7.0",
            build_file = "@//bindings/python:nanobind.BUILD",
            recursive_init_submodules = True,
        )

    if "libpfm" not in native.existing_rules():
        # Downloaded from v4.9.0 tag at https://sourceforge.net/p/perfmon2/libpfm4/ref/master/tags/
        http_archive(
            name = "libpfm",
            build_file = str(Label("//tools:libpfm.BUILD.bazel")),
            sha256 = "5da5f8872bde14b3634c9688d980f68bda28b510268723cc12973eedbab9fecc",
            type = "tar.gz",
            strip_prefix = "libpfm-4.11.0",
            urls = ["https://sourceforge.net/projects/perfmon2/files/libpfm4/libpfm-4.11.0.tar.gz/download"],
        )
