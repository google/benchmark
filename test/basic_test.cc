
#include "benchmark/benchmark.h"

using benchmark::StartBenchmarkTiming;
using benchmark::StopBenchmarkTiming;

void BM_empty(benchmark::State& state) {
  while (state.KeepRunning()) { }
}
BENCHMARK(BM_empty);
BENCHMARK(BM_empty)->ThreadPerCpu();

void BM_empty2(benchmark::State& state) {
  while (state.KeepRunning()) { }
}
BENCHMARK(BM_empty2)->ThreadPerCpu();

void BM_spin(benchmark::State& state) {
  while (state.KeepRunning()) {
    for (int x = 0; x < state.range_x(); ++x) {
      volatile int dummy = x;
      ((void)dummy);
    }
  }
}
BENCHMARK(BM_spin)->Range(8, 8<<10);

void BM_spin_pause_before(benchmark::State& state) {
  for (int i = 0; i < state.range_x(); ++i) {
    volatile int dummy = i;
    ((void)dummy);
  }
  while(state.KeepRunning()) {
    for (int i = 0; i < state.range_x(); ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
  }
}
BENCHMARK(BM_spin_pause_before)->Range(8, 8<<10);
BENCHMARK(BM_spin_pause_before)->Range(8, 8<<10)->ThreadPerCpu();


void BM_spin_pause_during(benchmark::State& state) {
  while(state.KeepRunning()) {
    StopBenchmarkTiming();
    for (int i = 0; i < state.range_x(); ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
    StartBenchmarkTiming();
    for (int i = 0; i < state.range_x(); ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
  }
}
BENCHMARK(BM_spin_pause_during)->Range(8, 8<<10);
BENCHMARK(BM_spin_pause_during)->Range(8, 8<<10)->ThreadPerCpu();

BENCHMARK_MAIN()
