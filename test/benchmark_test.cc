#include "benchmark/benchmark.h"

#include <math.h>
#include <stdint.h>

#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace {

int ATTRIBUTE_NOINLINE Factorial(uint32_t n) {
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
  for (int i = 0; i < size; ++i)
    s.insert(i);
  return s;
}

pthread_mutex_t test_vector_mu;
std::vector<int>* test_vector = nullptr;

}  // end namespace

#ifdef DEBUG
static void BM_Factorial(benchmark::State& state) {
  int fac_42 = 0;
  while (state.KeepRunning())
    fac_42 = Factorial(8);
  // Prevent compiler optimizations
  CHECK(fac_42 != std::numeric_limits<int>::max());
}
BENCHMARK(BM_Factorial);
#endif

static void BM_CalculatePiRange(benchmark::State& state) {
  double pi = 0.0;
  while (state.KeepRunning())
    pi = CalculatePi(state.range_x());
  std::stringstream ss;
  ss << pi;
  state.SetLabel(ss.str());
}
BENCHMARK_RANGE(BM_CalculatePiRange, 1, 1024 * 1024);

static void BM_CalculatePi(benchmark::State& state) {
  static const int depth = 1024;
  double pi ATTRIBUTE_UNUSED = 0.0;
  while (state.KeepRunning()) {
    pi = CalculatePi(depth);
  }
}
BENCHMARK(BM_CalculatePi)->Threads(8);
BENCHMARK(BM_CalculatePi)->ThreadRange(1, 32);
BENCHMARK(BM_CalculatePi)->ThreadPerCpu();

static void BM_SetInsert(benchmark::State& state) {
  while (state.KeepRunning()) {
    state.PauseTiming();
    std::set<int> data = ConstructRandomSet(state.range_x());
    state.ResumeTiming();
    for (int j = 0; j < state.range_y(); ++j)
      data.insert(rand());
  }
  state.SetItemsProcessed(state.iterations() * state.range_y());
  state.SetBytesProcessed(state.iterations() * state.range_y() * sizeof(int));
}
BENCHMARK(BM_SetInsert)->RangePair(1<<10,8<<10, 1,10);

template<typename Q>
static void BM_Sequential(benchmark::State& state) {
  typename Q::value_type v = 42;
  while (state.KeepRunning()) {
    Q q;
    for (int i = state.range_x(); --i; )
      q.push_back(v);
  }
  const int64_t items_processed =
      static_cast<int64_t>(state.iterations()) * state.range_x();
  state.SetItemsProcessed(items_processed);
  state.SetBytesProcessed(items_processed * sizeof(v));
}
BENCHMARK_TEMPLATE(BM_Sequential, std::vector<int>)->Range(1 << 0, 1 << 10);
BENCHMARK_TEMPLATE(BM_Sequential, std::list<int>)->Range(1 << 0, 1 << 10);

static void BM_StringCompare(benchmark::State& state) {
  std::string s1(state.range_x(), '-');
  std::string s2(state.range_x(), '-');
  int r = 0;
  while (state.KeepRunning())
    r |= s1.compare(s2);
  // Prevent compiler optimizations
  CHECK(r != std::numeric_limits<int>::max());
}
BENCHMARK(BM_StringCompare)->Range(1, 1<<20);

static void BM_SetupTeardown(benchmark::State& state) {
  if (state.thread_index == 0) {
    pthread_mutex_init(&test_vector_mu, nullptr);
    // No need to lock test_vector_mu here as this is running single-threaded.
    test_vector = new std::vector<int>();
  }
  int i = 0;
  while (state.KeepRunning()) {
    pthread_mutex_lock(&test_vector_mu);
    if (i%2 == 0)
      test_vector->push_back(i);
    else
      test_vector->pop_back();
    pthread_mutex_unlock(&test_vector_mu);
    ++i;
  }
  if (state.thread_index == 0) {
    delete test_vector;
    pthread_mutex_destroy(&test_vector_mu);
  }
}
BENCHMARK(BM_SetupTeardown)->ThreadPerCpu();

static void BM_LongTest(benchmark::State& state) {
  double tracker = 0.0;
  while (state.KeepRunning())
    for (int i = 0; i < state.range_x(); ++i)
      tracker += i;
  CHECK(tracker != 0.0);
}
BENCHMARK(BM_LongTest)->Range(1<<16,1<<28);

int main(int argc, const char* argv[]) {
  benchmark::Initialize(&argc, argv);

  CHECK(Factorial(8) == 40320);
  CHECK(CalculatePi(1) == 0.0);

  benchmark::RunSpecifiedBenchmarks();
}

