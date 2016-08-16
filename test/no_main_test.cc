
#include "benchmark/benchmark_api.h"

void BM_empty(benchmark::State& state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(state.iterations());
  }
}
BENCHMARK(BM_empty);

BENCHMARK_MAIN()
