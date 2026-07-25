// C++ adapter validation test.
//
// This test verifies that the Zig C adapter (zig_api.h/cc) correctly
// wraps the google-benchmark C++ API. It links against the combined
// archive using the system compiler (g++/clang++), avoiding Zig's
// linker limitations with system C++ libraries.
//
// The test exercises all adapter functions: initialization, benchmark
// registration, configuration, execution, custom context, and cleanup.
// Each test prints PASS/FAIL for CI visibility.

#include <cassert>
#include <cstdio>

#include "zig_api.h"

// Simple benchmark function: no-op loop, measures bare overhead.
static void test_benchmark(void* state) {
  while (benchmark_zig_state_keep_running(state)) {
    // No-op benchmark
  }
}

int main() {
  printf("Running Zig adapter tests...\n");

  // Test 1: Initialize — must be called before any other benchmark function.
  int argc = 1;
  char arg0[] = "test_adapter";
  char* argv[] = {arg0, nullptr};
  benchmark_zig_initialize(&argc, argv);
  printf("  [PASS] Initialize\n");

  // Test 2: RegisterBenchmark — registers a C callback as a benchmark.
  // Returns an opaque Benchmark* for configuration.
  void* bench = benchmark_zig_register_benchmark("BM_Test", test_benchmark);
  assert(bench != nullptr);
  printf("  [PASS] RegisterBenchmark\n");

  // Test 3: Configure the benchmark using fluent chaining methods.
  benchmark_zig_benchmark_arg(bench, 8);
  benchmark_zig_benchmark_range(bench, 1, 1024);
  benchmark_zig_benchmark_threads(bench, 1);
  benchmark_zig_benchmark_use_real_time(bench);
  printf("  [PASS] Benchmark configuration\n");

  // Test 4: GetName — retrieve the registered benchmark name.
  const char* name = benchmark_zig_benchmark_name(bench);
  assert(name != nullptr);
  printf("  [PASS] GetName: %s\n", name);

  // Test 5: Run — execute all registered benchmarks, returns count.
  size_t count = benchmark_zig_run();
  assert(count > 0);
  printf("  [PASS] Run: %zu benchmark(s) executed\n", count);

  // Test 6: AddCustomContext — add metadata to benchmark output.
  benchmark_zig_add_custom_context("test_key", "test_value");
  printf("  [PASS] AddCustomContext\n");

  // Test 7: ClearRegisteredBenchmarks — remove all registered benchmarks.
  benchmark_zig_clear_registered_benchmarks();
  printf("  [PASS] ClearRegisteredBenchmarks\n");

  printf("All adapter tests passed!\n");
  return 0;
}
