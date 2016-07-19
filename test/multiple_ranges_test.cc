#include "benchmark/benchmark.h"

void BM_empty(benchmark::State& state) {
  while (state.KeepRunning()) {
    int product = state.range(0) * state.range(1) * state.range(2);
    for (int x = 0; x < product; x++) {
      benchmark::DoNotOptimize(x);
    }
  }
}

BENCHMARK(BM_empty)->RangeMultiplier(2)->Ranges({1, 3, 5}, {2, 7, 15})->Args({7, 6, 3});

BENCHMARK_MAIN()
