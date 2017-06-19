#include "benchmark/benchmark_api.h"

#include <cassert>

static void BM_range_empty(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(state.iterations());
  }
}
BENCHMARK(BM_range_empty);
BENCHMARK(BM_range_empty)->ThreadPerCpu();

static void BM_range_iteration_count(benchmark::State& state) {
  size_t actual_iterations = 0;
  for (auto _ : state) {
    ++actual_iterations;
  }
  assert(state.iterations() == state.max_iterations);
  assert(state.iterations() == actual_iterations);
}
BENCHMARK(BM_range_iteration_count);
BENCHMARK(BM_range_iteration_count)->ThreadPerCpu();

static void BM_range_explicit_iteration_count(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(state.iterations());
  }
  assert(state.iterations() == state.max_iterations);
  assert(state.iterations() == 137);
}
BENCHMARK(BM_range_explicit_iteration_count)->Iterations(137);

BENCHMARK_MAIN()
