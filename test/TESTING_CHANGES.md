# Test Argument Management Changes

## Summary

Tests no longer require command-line arguments to be passed externally. All test arguments are now managed internally using the `AddTestArguments()` helper function from `default_arguments.h`.

## Motivation

Previously, tests required specific command-line arguments to be passed from the build system (CMakeLists.txt and BUILD files). This approach had several issues:

1. **Inconsistent enforcement**: Not all tests validated that required arguments were passed, making it easy to run tests incorrectly
2. **User confusion**: Users reported issues where tests failed because the correct arguments weren't provided
3. **Maintenance burden**: Arguments were duplicated between CMakeLists.txt and BUILD files and were not always in sync (e.g., perf_counters_test had different arguments in each)
4. **Poor developer experience**: Running tests manually required knowing which arguments to pass

## Solution

### New Header: `test/default_arguments.h`

A new header file provides the `AddTestArguments()` function that:

- Accepts custom test-specific arguments via initializer list
- Automatically adds `--benchmark_min_time=0.01s` to all tests for fast execution
- Warns users if they attempt to pass command-line arguments (since tests are now self-contained)
- Modifies `argc` and `argv` before they're used by the benchmark library

### Usage

In each test's `main()` function, call `AddTestArguments()` immediately after the function signature and before `MaybeReenterWithoutASLR()`:

```cpp
#include "default_arguments.h"

int main(int argc, char** argv) {
  AddTestArguments(argc, argv, {"--benchmark_filter=BM_MyTest"});
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  // ... rest of test
}
```

For tests without special arguments:

```cpp
int main(int argc, char** argv) {
  AddTestArguments(argc, argv);
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  // ... rest of test
}
```

### Default Arguments

All tests automatically receive:
- `--benchmark_min_time=0.01s` (for faster test execution)

### Test-Specific Arguments

Tests can add their own arguments:

| Test | Additional Arguments |
|------|---------------------|
| `spec_arg_test.cc` | `--benchmark_filter=BM_NotChosen` |
| `spec_arg_verbosity_test.cc` | `--v=42` |
| `user_counters_tabular_test.cc` | `--benchmark_counters_tabular=true` |
| `repetitions_test.cc` | `--benchmark_repetitions=3` |
| `complexity_test.cc` | `--benchmark_min_time=1000000x` (overrides default) |

## Build System Changes

### CMakeLists.txt

All explicit `--benchmark_min_time=0.01s` arguments have been removed from test commands since they're now added internally. Tests are now invoked simply as:

```cmake
benchmark_add_test(NAME my_test COMMAND my_test)
```

Exception: `filter_test` still receives its filter and expected count arguments since these are test parameters, not benchmark flags.

### BUILD (Bazel)

- `TEST_ARGS` is now empty (was `["--benchmark_min_time=0.01s"]`)
- `PER_SRC_TEST_ARGS` is now empty (all test-specific args are handled internally)

## Updated Files

### Core Infrastructure
- `test/default_arguments.h` (new)

### Test Files (25 total)
- `test/benchmark_min_time_flag_iters_test.cc`
- `test/benchmark_min_time_flag_time_test.cc`
- `test/benchmark_setup_teardown_test.cc`
- `test/complexity_test.cc`
- `test/diagnostics_test.cc`
- `test/display_aggregates_only_test.cc`
- `test/donotoptimize_test.cc`
- `test/filter_test.cc`
- `test/internal_threading_test.cc`
- `test/locale_impermeability_test.cc`
- `test/manual_threading_test.cc`
- `test/memory_manager_test.cc`
- `test/perf_counters_test.cc`
- `test/profiler_manager_iterations_test.cc`
- `test/profiler_manager_test.cc`
- `test/register_benchmark_test.cc`
- `test/repetitions_test.cc`
- `test/report_aggregates_only_test.cc`
- `test/reporter_output_test.cc`
- `test/skip_with_error_test.cc`
- `test/spec_arg_test.cc`
- `test/spec_arg_verbosity_test.cc`
- `test/user_counters_tabular_test.cc`
- `test/user_counters_test.cc`
- `test/user_counters_thousands_test.cc`

### Build Files
- `test/CMakeLists.txt`
- `test/BUILD`

## Benefits

1. **Self-contained tests**: Each test specifies exactly what it needs
2. **Reduced duplication**: Arguments defined once in the test source
3. **Better maintainability**: Changes to test requirements don't require build file updates
4. **Improved developer experience**: Tests can be run directly without remembering arguments
5. **Consistent behavior**: All tests follow the same pattern

## Backward Compatibility

Tests will display a warning if command-line arguments are provided:

```
Warning: Tests should not require command line arguments
```

This helps users transition to the new approach.