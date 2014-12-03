#include "benchmark/benchmark.h"

#include <gtest/gtest.h>

int main(int argc, const char* argv[]) {
  testing::InitGoogleTest(&argc, (char**) argv);
  benchmark::Initialize(&argc, argv);

  std::cout << "Running tests:\n";
  int result = RUN_ALL_TESTS();

  if (result == 0) {
    std::cout << "\nRunning benchmarks:\n";
    benchmark::RunSpecifiedBenchmarks();
  }

  return result;
}

