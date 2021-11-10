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
static void DoSetup1(const benchmark::State& state) {
  ++single::setup_call;

  // Setup/Teardown should never be called with any thread_idx != 0.
  assert(state.thread_index() == 0);
}

static void DoTeardown1(const benchmark::State& state) {
  ++single::teardown_call;
  assert(state.thread_index() == 0);
}

static int func_c = 0;
static void BM_with_setup(benchmark::State& state) {
  for (auto s : state) {
  }
}
BENCHMARK(BM_with_setup)
    ->Arg(1)
    ->Arg(3)
    ->Arg(5)
    ->Arg(7)
    ->Iterations(100)
    ->Setup(DoSetup1)
    ->Teardown(DoTeardown1);

// Test that Setup() and Teardown() are called once for each group of threads.
namespace concurrent {
static std::atomic<int> setup_call(0);
static std::atomic<int> teardown_call(0);
static std::atomic<int> func_call(0);
}  // namespace concurrent

static void DoSetup2(const benchmark::State& state) {
  concurrent::setup_call.fetch_add(1, std::memory_order_acquire);
  assert(state.thread_index() == 0);
}

static void DoTeardown2(const benchmark::State& state) {
  concurrent::teardown_call.fetch_add(1, std::memory_order_acquire);
  assert(state.thread_index() == 0);
}

static void BM_concurrent(benchmark::State& state) {
  for (auto s : state) {
  }
  concurrent::func_call.fetch_add(1, std::memory_order_acquire);
}

BENCHMARK(BM_concurrent)
    ->Setup(DoSetup2)
    ->Teardown(DoTeardown2)
    ->Iterations(100)
    ->Threads(5)
    ->Threads(10)
    ->Threads(15);

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);

  size_t ret = benchmark::RunSpecifiedBenchmarks(".");
  assert(ret > 0);

  // Setup/Teardown is called once for each arg group.
  // (There were 3,5,7,4)
  assert(single::setup_call == 4);
  assert(single::teardown_call == 4);

  assert(concurrent::setup_call.load(std::memory_order_relaxed) == 3);
  assert(concurrent::teardown_call.load(std::memory_order_relaxed) == 3);

  // 3 group of threads calling this function.
  assert((5 + 10 + 15) ==
         concurrent::func_call.load(std::memory_order_relaxed));

  return 0;
}

#else
// Do nothing
int main(int, char**) { return 0; }
#endif  // BENCHMARK_HAS_CXX11
