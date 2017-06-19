#include "benchmark/benchmark.h"

#include <algorithm>
#include <cassert>

static void BM_batch_iteration_count(benchmark::State& state) {
  const size_t batch_size = state.range(0);
  size_t actual_iterations = 0;
  while (state.KeepRunningBatch(batch_size)) {
    actual_iterations += batch_size;
  }
  assert(state.iterations() >= state.max_iterations);
  assert(state.iterations() == actual_iterations);
}
BENCHMARK(BM_batch_iteration_count)->Range(8, 8 << 10);
BENCHMARK(BM_batch_iteration_count)->Range(8, 8 << 10)->ThreadPerCpu();

bool ParameterizedRoutine(size_t param) {
  std::vector<int> values;
  for (size_t i = 0; i < param; ++i) values.push_back(i);
  auto spin_compare = [](int a, int b) -> bool {
    for (int i = 1; i < 2048; ++i) {
      benchmark::DoNotOptimize(i);
    }
    return a < b;
  };
  return std::binary_search(values.begin(), values.end(), -1, spin_compare);
}

static void BM_batch_basic(benchmark::State& state) {
  const size_t batch_size = state.range(0);
  while (state.KeepRunningBatch(batch_size)) {
    benchmark::DoNotOptimize(ParameterizedRoutine(batch_size));
  }
}
BENCHMARK(BM_batch_basic)->Range(1, 1 << 10);

BENCHMARK_MAIN()
