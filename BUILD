cc_library(
    name = "benchmark",
    srcs = ["src/benchmark.cc"],
    hdrs = ["include/benchmark/benchmark.h"],
    strip_include_prefix = "include",
    deps = [
        ":benchmark_api_internal",
        ":benchmark_header",
        ":benchmark_register",
        ":check",
        ":colorprint",
        ":commandlineflags",
        ":complexity",
        ":console_reporter",
        ":counter",
        ":csv_reporter",
        ":internal_macros",
        ":json_reporter",
        ":log",
        ":mutex",
        ":re",
        ":reporter",
        ":statistics",
        ":string_util",
        ":sysinfo",
        ":timers",
    ],
    visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "arraysize",
    hdrs = ["src/arraysize.h"],
    strip_include_prefix = "src",
 )

cc_library(
    name = "benchmark_api_internal",
    hdrs = ["src/benchmark_api_internal.h"],
    strip_include_prefix = "src",
)

cc_library(
    name = "benchmark_header",
    hdrs = ["include/benchmark/benchmark.h"],
    strip_include_prefix = "include",
 )

cc_library(
    name = "benchmark_register",
    srcs = ["src/benchmark_register.cc"],
    deps = [
        ":benchmark_api_internal",
        ":benchmark_header",
        ":check",
        ":commandlineflags",
        ":complexity",
        ":internal_macros",
        ":mutex",
        ":re",
        ":statistics",
        ":string_util",
        ":timers",
    ],
 )

cc_library(
    name = "check",
    hdrs = ["src/check.h"],
    strip_include_prefix = "src",
    deps = [
        ":benchmark_header",
        ":internal_macros",
        ":log",
    ],
)

cc_library(
    name = "colorprint",
    hdrs = ["src/colorprint.h"],
    srcs = ["src/colorprint.cc"],
    strip_include_prefix = "src",
    deps = [
        ":check",
        ":internal_macros",
    ]
)

cc_library(
    name = "commandlineflags",
    hdrs = ["src/commandlineflags.h"],
    srcs = ["src/commandlineflags.cc"],
    strip_include_prefix = "src",
    deps = [
    ]
)

cc_library(
    name = "complexity",
    hdrs = ["src/complexity.h"],
    srcs = ["src/complexity.cc"],
    strip_include_prefix = "src",
    deps = [
        ":benchmark_header",
        ":check",
    ]
)

cc_library(
    name = "console_reporter",
    srcs = ["src/console_reporter.cc"],
    strip_include_prefix = "src",
    deps = [
        ":benchmark_header",
        ":commandlineflags",
        ":complexity",
        ":colorprint",
        ":counter",
        ":string_util",
        ":timers",
    ]
)

cc_library(
    name = "counter",
    hdrs = ["src/counter.h"],
    srcs = ["src/counter.cc"],
    strip_include_prefix = "src",
    deps = [
        ":benchmark_header",
    ]
)

cc_library(
    name = "csv_reporter",
    srcs = ["src/csv_reporter.cc"],
    strip_include_prefix = "src",
    deps = [
        ":benchmark_header",
        ":complexity",
        ":string_util",
        ":timers",
    ]
)

cc_library(
    name = "cycleclock",
    hdrs = ["src/cycleclock.h"],
    strip_include_prefix = "src",
    deps = [
    ]
)
cc_library(
    name = "internal_macros",
    hdrs = ["src/internal_macros.h"],
    strip_include_prefix = "src",
    deps = [
        ":benchmark_header",
    ],
)

cc_library(
    name = "json_reporter",
    srcs = ["src/json_reporter.cc"],
    strip_include_prefix = "src",
    deps = [
        ":benchmark_header",
        ":complexity",
        ":string_util",
        ":timers",
    ]
)

cc_library(
    name = "log",
    hdrs = ["src/log.h"],
    strip_include_prefix = "src",
)

cc_library(
    name = "mutex",
    hdrs = ["src/mutex.h"],
    strip_include_prefix = "src",
    deps = [
    ]
)

cc_library(
    name = "re",
    hdrs = ["src/re.h"],
    strip_include_prefix = "src",
    deps = [
    ],
    defines = ["HAVE_POSIX_REGEX"],
)

cc_library(
    name = "reporter",
    srcs = ["src/reporter.cc"],
    strip_include_prefix = "src",
    deps = [
        ":benchmark_header",
        ":timers",
    ]
)

cc_library(
    name = "sleep",
    hdrs = ["src/sleep.h"],
    srcs = ["src/sleep.cc"],
    strip_include_prefix = "src",
    deps = [
        ":internal_macros",
    ]
)

cc_library(
    name = "statistics",
    hdrs = ["src/statistics.h"],
    srcs = ["src/statistics.cc"],
    strip_include_prefix = "src",
    deps = [
        ":benchmark_header",
        ":check",
    ]
)

cc_library(
    name = "string_util",
    hdrs = ["src/string_util.h"],
    srcs = ["src/string_util.cc"],
    strip_include_prefix = "src",
    deps = [
        ":arraysize",
        ":internal_macros",
    ]
)

cc_library(
    name = "sysinfo",
    srcs = ["src/sysinfo.cc"],
    strip_include_prefix = "src",
    deps = [
        ":check",
        ":cycleclock",
        ":internal_macros",
        ":sleep",
        ":string_util",
    ]
)

cc_library(
    name = "timers",
    hdrs = ["src/timers.h"],
    srcs = ["src/timers.cc"],
    strip_include_prefix = "src",
    deps = [
        ":check",
        ":internal_macros",
        ":sleep",
        ":string_util",
    ]
)

cc_test(
    name = "basic_test",
    srcs = ["test/basic_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "benchmark_test",
    srcs = ["test/benchmark_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "complexity_test",
    srcs = ["test/complexity_test.cc"],
    deps = [
        ":benchmark",
        ":output_test",
    ],
)

#cc_test(
#    name = "cxx03_test",
#    srcs = ["test/cxx03_test.cc"],
#    deps = [":benchmark"],
#)

cc_test(
    name = "diagnostics_test",
    srcs = ["test/diagnostics_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "donotoptimize_test",
    srcs = ["test/donotoptimize_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "filter_test",
    srcs = ["test/filter_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "fixture_test",
    srcs = ["test/fixture_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "map_test",
    srcs = ["test/map_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "multiple_ranges_test",
    srcs = ["test/multiple_ranges_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "options_test",
    srcs = ["test/options_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "register_benchmark_test",
    srcs = ["test/register_benchmark_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "reporter_output_test",
    srcs = ["test/reporter_output_test.cc"],
    deps = [
        ":benchmark",
        ":output_test",
    ],
)

cc_test(
    name = "skip_with_error_test",
    srcs = ["test/skip_with_error_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "templated_fixture_test",
    srcs = ["test/templated_fixture_test.cc"],
    deps = [":benchmark"],
)

cc_test(
    name = "user_counters_tabular_test",
    srcs = ["test/user_counters_tabular_test.cc"],
    deps = [
        ":benchmark",
        ":output_test",
    ],
)

cc_test(
    name = "user_counters_test",
    srcs = ["test/user_counters_test.cc"],
    deps = [
        ":benchmark",
        ":output_test",
    ],
)

cc_library(
    name = "output_test",
    hdrs = ["test/output_test.h"],
    srcs = ["test/output_test_helper.cc"],
    strip_include_prefix = "test",
    deps = [
        ":benchmark_api_internal",
        ":check",
        ":re",
    ],
)
    

