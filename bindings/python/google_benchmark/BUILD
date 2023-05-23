load("@rules_python//python:packaging.bzl", "py_package", "py_wheel")
load("//bindings/python:build_defs.bzl", "py_extension")

exports_files(glob(["*.BUILD"]))
exports_files(["build_defs.bzl"])

config_setting(
    name = "linux_aarch64_release",
    values = {"cpu": "linux-aarch_64"},
)

config_setting(
    name = "linux_aarch64_local",
    constraint_values = [
        "@platforms//os:linux",
        "@platforms//cpu:aarch64",
    ],
)

config_setting(
    name = "linux_x86_64_release",
    values = {"cpu": "linux-x86_64"},
)

config_setting(
    name = "linux_x86_64_local",
    constraint_values = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
    ],
)

config_setting(
    name = "osx_x86_64_release",
    values = {"cpu": "osx-x86_64"},
)

config_setting(
    name = "osx_x86_64_local",
    constraint_values = [
        "@platforms//os:osx",
        "@platforms//cpu:x86_64",
    ],
)

py_library(
    name = "google_benchmark",
    srcs = ["__init__.py"],
    visibility = ["//visibility:public"],
    deps = [
        ":_benchmark",
    ],
)

py_extension(
    name = "_benchmark",
    srcs = ["benchmark.cc"],
    copts = [
        "-fexceptions",
        "-fno-strict-aliasing",
        "--std=c++17", # needed to use nanobind
    ],
    features = [
        "-use_header_modules",
        "-parse_headers",
    ],
    deps = [
        "//:benchmark",
        "@nanobind//:nanobind",
    ],
)

py_test(
    name = "example",
    srcs = ["example.py"],
    python_version = "PY3",
    srcs_version = "PY3",
    visibility = ["//visibility:public"],
    deps = [
        ":google_benchmark",
    ],
)

py_package(
    name = "pkg",
    # Only include these Python packages.
    packages = ["examples.wheel"],
    deps = [":google_benchmark"],
)

py_wheel(
    name = "google-benchmark",
    version = "", # TODO
    author = "Google",
    author_email = "benchmark-py@google.com",
    classifiers = [
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: Apache Software License",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Software Development :: Testing",
        "Topic :: System :: Benchmark",
    ],
    distribution = "google-benchmark",
    homepage = "https://github.com/google/benchmark",
    license = "Apache 2.0",
    platform = select({
        ":linux_x86_64_local": "linux_x86_64",
        ":linux_x86_64_release": "manylinux2014_x86_64",
        ":linux_aarch64_local": "linux_aarch64",
        ":linux_aarch64_release": "manylinux2014_aarch64",
        #":osx_universal2": "macosx_10_9_universal2",
        #":osx_aarch64": "macosx_11_0_arm64",
        #":windows_x86_32": "win32",
        #":windows_x86_64": "win_amd64",
        "//conditions:default": "any",
    }),
    python_requires = ">=3.8",
    requires = [
        "absl-py>=0.7.1",
    ],
    deps = [
        ":pkg",
    ],
)


