
#include <cstddef>

#include "benchmark/benchmark_api.h"

#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)->Arg(8)->Arg(512)->Arg(8192)

void BM_empty(benchmark::State& state) {
  while (state.KeepRunning()) {
    volatile std::size_t x = state.iterations();
    ((void)x);
  }
}
BENCHMARK(BM_empty);
BENCHMARK(BM_empty)->ThreadPerCpu();

void BM_spin_empty(benchmark::State& state) {
  while (state.KeepRunning()) {
    for (int x = 0; x < state.range_x(); ++x) {
      volatile int dummy = x;
      ((void)dummy);
    }
  }
}
BASIC_BENCHMARK_TEST(BM_spin_empty);
BASIC_BENCHMARK_TEST(BM_spin_empty)->ThreadPerCpu();

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
BASIC_BENCHMARK_TEST(BM_spin_pause_before);
BASIC_BENCHMARK_TEST(BM_spin_pause_before)->ThreadPerCpu();


void BM_spin_pause_during(benchmark::State& state) {
  while(state.KeepRunning()) {
    state.PauseTiming();
    for (int i = 0; i < state.range_x(); ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
    state.ResumeTiming();
    for (int i = 0; i < state.range_x(); ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_during);
BASIC_BENCHMARK_TEST(BM_spin_pause_during)->ThreadPerCpu();


void BM_spin_pause_after(benchmark::State& state) {
  while(state.KeepRunning()) {
    for (int i = 0; i < state.range_x(); ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
  }
  for (int i = 0; i < state.range_x(); ++i) {
    volatile int dummy = i;
    ((void)dummy);
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_after);
BASIC_BENCHMARK_TEST(BM_spin_pause_after)->ThreadPerCpu();


void BM_spin_pause_before_and_after(benchmark::State& state) {
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
  for (int i = 0; i < state.range_x(); ++i) {
    volatile int dummy = i;
    ((void)dummy);
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_before_and_after);
BASIC_BENCHMARK_TEST(BM_spin_pause_before_and_after)->ThreadPerCpu();


void BM_empty_stop_start(benchmark::State& state) {
  while (state.KeepRunning()) { }
}
BENCHMARK(BM_empty_stop_start);
BENCHMARK(BM_empty_stop_start)->ThreadPerCpu();

BENCHMARK_MAIN()
