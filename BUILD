licenses(['notice'])
package(default_visibility=['//visibility:public'])

BENCHMARK_COPTS = [
    # Choose the regex backend by defining one of the macros below:
    #       HAVE_GNU_POSIX_REGEX
    #       HAVE_POSIX_REGEX
    #       HAVE_STD_REGEX
    '-DHAVE_POSIX_REGEX',
    '-UNDEBUG',
    '-Wall',
    '-Wextra',
    '-Wfloat-equal',
    '-Wshadow',
    '-Wstrict-aliasing',
    '-Wzero-as-null-pointer-constant',
    '-fstrict-aliasing',
    '-pedantic',
    '-pedantic-errors',
    '-std=c++11',
]


GTEST_DEP = '@com_github_google_googletest//:gtest'

cc_library(
    name = 'benchmark',
    srcs = [
        'src/arraysize.h',
        'src/benchmark.cc',
        'src/benchmark_api_internal.h',
        'src/benchmark_register.cc',
        'src/check.h',
        'src/colorprint.cc',
        'src/colorprint.h',
        'src/commandlineflags.cc',
        'src/commandlineflags.h',
        'src/complexity.cc',
        'src/complexity.h',
        'src/console_reporter.cc',
        'src/counter.cc',
        'src/counter.h',
        'src/csv_reporter.cc',
        'src/cycleclock.h',
        'src/internal_macros.h',
        'src/json_reporter.cc',
        'src/log.h',
        'src/mutex.h',
        'src/re.h',
        'src/reporter.cc',
        'src/sleep.cc',
        'src/sleep.h',
        'src/statistics.cc',
        'src/statistics.h',
        'src/string_util.cc',
        'src/string_util.h',
        'src/sysinfo.cc',
        'src/timers.cc',
        'src/timers.h',
    ],
    hdrs = [
        'include/benchmark/benchmark.h',
    ],
    includes = [
        'include',
    ],
    copts = BENCHMARK_COPTS,
    linkopts = [
        '-lm',
        '-pthread',
    ],
)

[cc_test(
    name = f,
    srcs = [
        'test/%s.cc' % f,
    ],
    copts = BENCHMARK_COPTS,
    deps = [
        ':benchmark',
    ],
    testonly = 1,
) for f in [
        'basic_test',
        'benchmark_test',
        'diagnostics_test',
        'donotoptimize_test',
        'filter_test',
        'fixture_test',
        'map_test',
        'multiple_ranges_test',
        'options_test',
        'register_benchmark_test',
        'skip_with_error_test',
        'templated_fixture_test',
    ]
]

[cc_test(
    name = f,
    srcs = [
        'test/%s.cc' % f,
    ],
    copts = BENCHMARK_COPTS,
    deps = [
        ':output_test_helper',
        GTEST_DEP,
    ],
    testonly = 1,
) for f in [
        'complexity_test',
        'reporter_output_test',
        'user_counters_tabular_test',
        'user_counters_test',
    ]
]

cc_test(
    name = 'cxx03_test',
    srcs = [
        'test/cxx03_test.cc',
    ],
    copts = [
        '-std=c++03',
    ],
    deps = [
        ':benchmark',
    ],
    testonly = 1,
)

cc_library(
    name = 'output_test_helper',
    visibility = [
        '//visibility:__pkg__',
    ],
    srcs = [
        'test/output_test_helper.cc',
    ],
    hdrs = [
        'test/output_test.h',
    ],
    strip_include_prefix = 'test',
    copts = BENCHMARK_COPTS,
    deps = [
        ':benchmark',
    ],
    testonly = 1,
)
