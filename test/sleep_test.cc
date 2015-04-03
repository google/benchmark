
#include <cstddef>
#include <chrono>
#include <thread>

#include "benchmark/benchmark_api.h"

void BM_sleep(benchmark::State& state) {
  std::chrono::nanoseconds ns(10000);
  while (state.KeepRunning()) {
    std::this_thread::sleep_for(ns);
  }
}
BENCHMARK(BM_sleep);

BENCHMARK_MAIN()
