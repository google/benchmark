#include "benchmark/benchmark.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(__GNUC__)
#define BENCHMARK_NOINLINE __attribute__((noinline))
#else
#define BENCHMARK_NOINLINE
#endif

namespace {

int BENCHMARK_NOINLINE Factorial(uint32_t n) {
  return (n == 1) ? 1 : n * Factorial(n - 1);
}

double CalculatePi(int depth) {
  double pi = 0.0;
  for (int i = 0; i < depth; ++i) {
    double numerator = static_cast<double>(((i % 2) * 2) - 1);
    double denominator = static_cast<double>((2 * i) - 1);
    pi += numerator / denominator;
  }
  return (pi - 1.0) * 4;
}

std::set<int> ConstructRandomSet(int size) {
  std::set<int> s;
  for (int i = 0; i < size; ++i) s.insert(i);
  return s;
}

std::vector<int>* test_vector = nullptr;

}  // end namespace

static void BM_Factorial(benchmark::State& state) {
  int fac_42 = 0;
  while (state.KeepRunning()) fac_42 = Factorial(8);
  // Prevent compiler optimizations
  std::stringstream ss;
  ss << fac_42;
  state.SetLabel(ss.str());
}
// FIXME Highcharts visualization (enabled by HTMLReporter) currently is working only with ranges/range pairs
BENCHMARK(BM_Factorial)->UseRealTime()->Ranges({{1 << 10, 8 << 10}, {1, 10}});


static void BM_CalculatePi(benchmark::State& state) {
  static const int depth = 1024;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(CalculatePi(depth));
  }
}
// FIXME Highcharts visualization (enabled by HTMLReporter) currently is working only with ranges/range pairs 
BENCHMARK(BM_CalculatePi)->Threads(8)->Ranges({{1 << 10, 8 << 10}, {1, 10}});
BENCHMARK(BM_CalculatePi)->ThreadRange(1, 32)->Ranges({{1 << 10, 8 << 10}, {1, 10}});
BENCHMARK(BM_CalculatePi)->ThreadPerCpu()->Ranges({{1 << 10, 8 << 10}, {1, 10}});

static void BM_SetInsert(benchmark::State& state) {
  while (state.KeepRunning()) {
    state.PauseTiming();
    std::set<int> data = ConstructRandomSet(state.range(0));
    state.ResumeTiming();
    for (int j = 0; j < state.range(1); ++j) data.insert(rand());
  }
  state.SetItemsProcessed(state.iterations() * state.range(1));
  state.SetBytesProcessed(state.iterations() * state.range(1) * sizeof(int));
}
// FIXME Highcharts visualization (enabled by HTMLReporter) currently is working only with ranges/range pairs
BENCHMARK(BM_SetInsert)->RangePair(1<<10,8<<10,1,10);

static void BM_ParallelMemset(benchmark::State& state) {
  int size = state.range(0) / static_cast<int>(sizeof(int));
  int thread_size = size / state.threads;
  int from = thread_size * state.thread_index;
  int to = from + thread_size;

  if (state.thread_index == 0) {
    test_vector = new std::vector<int>(size);
  }

  while (state.KeepRunning()) {
    for (int i = from; i < to; i++) {
      // No need to lock test_vector_mu as ranges
      // do not overlap between threads.
      benchmark::DoNotOptimize(test_vector->at(i) = 1);
    }
  }

  if (state.thread_index == 0) {
    delete test_vector;
  }
}
// FIXME Highcharts visualization (enabled by HTMLReporter) currently is working only with ranges/range pairs
BENCHMARK(BM_ParallelMemset)->ThreadRange(1, 4)->Ranges({{1 << 10, 8 << 10}, {1, 10}});

BENCHMARK_MAIN()
