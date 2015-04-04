
#include <cstddef>
#include <chrono>
#include <thread>
#include <stdio.h>
#include "benchmark/benchmark_api.h"

#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)->Arg(10)->Arg(100)->Arg(10000)->Arg(500000)->\
                  Arg(1000000)->Arg(5000000)->Arg(1000000000)

#define CHECK_EXPECTED_BENCHMARK \
  char f[100];\
  sprintf(f, "Best(ns) score is: %6.0f%% of expected performance", state.best_performance()*100/state.range_x());\
  state.SetLabel(f);\

void BM_sleep(benchmark::State& state) {
  std::chrono::nanoseconds ns(state.range_x());

  while (state.KeepRunning()) {
    std::this_thread::sleep_for(ns);
  }

  CHECK_EXPECTED_BENCHMARK
}

BASIC_BENCHMARK_TEST(BM_sleep);
BASIC_BENCHMARK_TEST(BM_sleep)->UseRealTime();
BASIC_BENCHMARK_TEST(BM_sleep)->ThreadPerCpu();

void BM_sleep_and_wait(benchmark::State& state) {
  std::chrono::nanoseconds ns(state.range_x());

  while (state.KeepRunning()) {
    state.PauseTiming();
    {
      /* this loop will be not be counted */
      for (int i = 0; i < state.range_y(); ++i) {
        benchmark::DoNotOptimize(i);
      }
    }
    state.ResumeTiming();

    std::this_thread::sleep_for(ns);
  }
}

BENCHMARK(BM_sleep_and_wait)->ArgPair(100000000, 1<<10);

void BM_real_sleep_and_virtual_sleep(benchmark::State& state) {
  std::chrono::nanoseconds ns_real(state.range_x());
  std::chrono::nanoseconds ns_virtual(state.range_y());

  while (state.KeepRunning()) {
    state.PauseTiming();
    {
      /* this sleep will not be counted */
      std::this_thread::sleep_for(ns_virtual);
    }
    state.ResumeTiming();

    std::this_thread::sleep_for(ns_real);

    state.PauseTiming();
    {
      /* this sleep will not be counted */
      std::this_thread::sleep_for(ns_virtual);
    }
    state.ResumeTiming();

  }
}

BENCHMARK(BM_real_sleep_and_virtual_sleep)->ArgPair(1000000000, 2000000000);

BENCHMARK_MAIN()
