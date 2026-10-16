load("@nanobind_bazel//:build_defs.bzl", "nanobind_extension", "nanobind_stubgen")
load("@rules_python//python:defs.bzl", "py_library", "py_test")

py_library(
    name = "google_benchmark",
    srcs = ["__init__.py"],
    data = [":_benchmark"],
    imports = [".."],
    visibility = ["//visibility:public"],
    deps = select({
        "@nanobind_bazel//:with_split_mode": ["@nanobind_bazel//:nanobind_backend"],
        "//conditions:default": [],
    }),
)

nanobind_extension(
    name = "_benchmark",
    srcs = ["benchmark.cc"],
    deps = ["//:benchmark"],
)

nanobind_stubgen(
    name = "benchmark_stubgen",
    module = ":_benchmark",
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
