#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>

#include "benchmark/benchmark.h"

#ifdef BENCHMARK_HAS_CXX11

// Test that Setup() and Teardown() are called exactly once
// for each benchmark run (single-threaded).
namespace single {
static int setup_call = 0;
static int teardown_call = 0;
}  // namespace single
static void DoSetup1(benchmark::State& state) {
  ++single::setup_call;

  // Setup/Teardown should never be called with any thread_idx != 0.
  assert(state.thread_index() == 0);
}

static void DoTeardown1(benchmark::State& state) {
  ++single::teardown_call;
  assert(state.thread_index() == 0);
}

static void BM_with_setup(benchmark::State& state) {
  for (auto s : state) {
  }
}
BENCHMARK(BM_with_setup)
    ->Arg(1)
    ->Arg(3)
    ->Arg(5)
    ->Setup(DoSetup1)
    ->Teardown(DoTeardown1);

// Test that Setup() and Teardown() are called once for each group of threads.
namespace concurrent {
static std::atomic<int> setup_call(0);
static std::atomic<int> teardown_call(0);
static std::atomic<int> func_call(0);
}  // namespace concurrent

static void DoSetup2(benchmark::State& state) {
  concurrent::setup_call.fetch_add(1, std::memory_order_acquire);
  assert(state.thread_index() == 0);
}

static void DoTeardown2(benchmark::State& state) {
  concurrent::teardown_call.fetch_add(1, std::memory_order_acquire);
  assert(state.thread_index() == 0);
}

static void BM_concurrent(benchmark::State& state) {
  for (auto s : state) {
  }
  concurrent::func_call.fetch_add(1, std::memory_order_acquire);
}

BENCHMARK(BM_concurrent)->Threads(5)->Threads(10)->Threads(15);

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);

  size_t ret = benchmark::RunSpecifiedBenchmarks(".");
  assert(ret > 0);

  assert(single::setup_call == 1);
  assert(single::teardown_call == 1);

  assert(concurrent::setup_call.load(memory_order_relaxed) == 3);
  assert(concurent::teardown_call.load(memory_order_relaxed) == 3);

  // 3 group of threads calling this function.
  assert((5 + 10 + 15) == concurrent::func_call.load(memory_order_relaxed));

  return 0;
}

#else
// Do nothing
int main(int, char**) { return 0; }
#endif  // BENCHMARK_HAS_CXX11
