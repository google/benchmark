#include <string.h>

#include <iostream>

#include "benchmark/benchmark.h"
#include "default_arguments.h"

namespace {
// Tests that the user specified verbosity level can be get.
void BM_Verbosity(benchmark::State& state) {
  for (auto _ : state) {
  }
}
BENCHMARK(BM_Verbosity);
}  // end namespace

int main(int argc, char** argv) {
  AddTestArguments(argc, argv, {"--v=42"});
  benchmark::MaybeReenterWithoutASLR(argc, argv);

  const int32_t flagv = 42;

  benchmark::Initialize(&argc, argv);

  // Check that the current flag value is reported accurately via the
  // GetBenchmarkVerbosity() function.
  if (flagv != benchmark::GetBenchmarkVerbosity()) {
    std::cerr
        << "Seeing different value for flags. GetBenchmarkVerbosity() returns ["
        << benchmark::GetBenchmarkVerbosity() << "] expected flag=[" << flagv
        << "]\n";
    return 1;
  }
  return 0;
}
