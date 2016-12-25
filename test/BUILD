cc_library(
    name = "output_test_helper",
    srcs = [
        "output_test.h",
        "output_test_helper.cc",
    ],
    deps = ["//:benchmark"],
)

cc_binary(
    name = "basic_test",
    srcs = ["basic_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "benchmark_test",
    srcs = ["benchmark_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "complexity_test",
    srcs = ["complexity_test.cc"],
    deps = [
        ":output_test_helper",
        "//:benchmark",
    ],
    linkopts = ["-lm", "-pthread"],
)

# cc_binary(
#     name = "cxx03_test",
#     srcs = ["cxx03_test.cc"],
#     deps = ["//:benchmark"],
# )

cc_binary(
    name = "diagnostics_test",
    srcs = ["diagnostics_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "donotoptimize_test",
    srcs = ["donotoptimize_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "filter_test",
    srcs = ["filter_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "fixture_test",
    srcs = ["fixture_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "map_test",
    srcs = ["map_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "options_test",
    srcs = ["options_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "register_benchmark_test",
    srcs = ["register_benchmark_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "reporter_output_test",
    srcs = ["reporter_output_test.cc"],
    deps = [
        ":output_test_helper",
        "//:benchmark",
    ],
    linkopts = ["-lm", "-pthread"],
)

cc_binary(
    name = "skip_with_error_test",
    srcs = ["skip_with_error_test.cc"],
    deps = ["//:benchmark"],
    linkopts = ["-lm", "-pthread"],
)

