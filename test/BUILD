NEEDS_GTEST_MAIN = [
    "statistics_test.cc",
]

TEST_COPTS = [
    "-pedantic",
    "-pedantic-errors",
    "-std=c++11",
]

TEST_ARGS = ["--benchmark_min_time=0.01"]

cc_library(
    name = "output_test_helper",
    testonly = 1,
    srcs = ["output_test_helper.cc"],
    hdrs = ["output_test.h"],
    copts = TEST_COPTS,
    deps = [
        "//:benchmark",
        "//:benchmark_internal_headers",
    ],
)

[cc_test(
    name = test_src[:-len(".cc")],
    size = "small",
    srcs = [test_src],
    args = TEST_ARGS + ({
        "user_counters_tabular_test.cc": ["--benchmark_counters_tabular=true"],
    }).get(test_src, []),
    copts = TEST_COPTS + ({
        "cxx03_test.cc": ["-std=c++03"],
        # Some of the issues with DoNotOptimize only occur when optimization is enabled
        "donotoptimize_test.cc": ["-O3"],
    }).get(test_src, []),
    deps = [
        ":output_test_helper",
        "//:benchmark",
        "//:benchmark_internal_headers",
        "@com_google_googletest//:gtest",
    ] + (
        ["@com_google_googletest//:gtest_main"] if (test_src in NEEDS_GTEST_MAIN) else []
    ),
# FIXME: Add support for assembly tests to bazel.
# See Issue #556
# https://github.com/google/benchmark/issues/556
) for test_src in glob(["*_test.cc"], exclude = ["*_assembly_test.cc"])]
