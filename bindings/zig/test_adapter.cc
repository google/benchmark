// Minimal C++ test that validates the Zig adapter works correctly.
// This test links against the combined archive using the system compiler,
// avoiding Zig's linker limitations with system C++ libraries.

#include <cassert>
#include <cstdio>

#include "zig_api.h"

// Simple benchmark function for testing
static void test_benchmark(void* state) {
  while (benchmark_zig_state_keep_running(state)) {
    // No-op benchmark
  }
}

int main() {
  printf("Running Zig adapter tests...\n");

  // Test 1: Initialize
  int argc = 1;
  char arg0[] = "test_adapter";
  char* argv[] = {arg0, nullptr};
  benchmark_zig_initialize(&argc, argv);
  printf("  [PASS] Initialize\n");

  // Test 2: Register benchmark
  void* bench = benchmark_zig_register_benchmark("BM_Test", test_benchmark);
  assert(bench != nullptr);
  printf("  [PASS] RegisterBenchmark\n");

  // Test 3: Configure benchmark
  benchmark_zig_benchmark_arg(bench, 8);
  benchmark_zig_benchmark_range(bench, 1, 1024);
  benchmark_zig_benchmark_threads(bench, 1);
  benchmark_zig_benchmark_use_real_time(bench);
  printf("  [PASS] Benchmark configuration\n");

  // Test 4: Get benchmark name
  const char* name = benchmark_zig_benchmark_name(bench);
  assert(name != nullptr);
  printf("  [PASS] GetName: %s\n", name);

  // Test 5: Run benchmarks
  size_t count = benchmark_zig_run();
  assert(count > 0);
  printf("  [PASS] Run: %zu benchmark(s) executed\n", count);

  // Test 6: Add custom context
  benchmark_zig_add_custom_context("test_key", "test_value");
  printf("  [PASS] AddCustomContext\n");

  // Test 7: Clear registered benchmarks
  benchmark_zig_clear_registered_benchmarks();
  printf("  [PASS] ClearRegisteredBenchmarks\n");

  printf("All adapter tests passed!\n");
  return 0;
}
