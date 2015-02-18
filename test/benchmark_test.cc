#include "benchmark/benchmark.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using benchmark::StartBenchmarkTiming;
using benchmark::StopBenchmarkTiming;
using benchmark::SetBenchmarkBytesProcessed;
using benchmark::SetBenchmarkItemsProcessed;

namespace {

#ifdef DEBUG
int ATTRIBUTE_NOINLINE Factorial(uint32_t n) {
  return (n == 1) ? 1 : n * Factorial(n - 1);
}
#endif

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

std::mutex test_vector_mu;
std::vector<int>* test_vector = nullptr;
static bool setup_called = false;

}  // end namespace

#ifdef DEBUG
static void BM_Factorial(int iters) {
  int fac_42 = 0;
  while (iters-- > 0)
    fac_42 = Factorial(8);
  // Prevent compiler optimizations
  EXPECT_NE(fac_42, std::numeric_limits<int>::max());
}
BENCHMARK(BM_Factorial);
#endif

static void BM_CalculatePiRange(int iters, int rangex) {
  double pi = 0.0;
  while (iters-- > 0)
    pi = CalculatePi(rangex);
  std::stringstream ss;
  ss << pi;
  //state.SetLabel(ss.str().c_str());
}
BENCHMARK_RANGE(BM_CalculatePiRange, 1, 1024 * 1024);

static void BM_CalculatePi(int iters) {
  static const int depth = 1024;
  double pi ATTRIBUTE_UNUSED = 0.0;
  while (iters-- > 0) {
    pi = CalculatePi(depth);
  }
}
BENCHMARK(BM_CalculatePi)->Threads(8);
BENCHMARK(BM_CalculatePi)->ThreadRange(1, 32);
BENCHMARK(BM_CalculatePi)->ThreadPerCpu();


static void BM_SetInsert(int iters, int xrange, int yrange) {
  const int total_iters = iters;
  while (iters-->0) {
    StopBenchmarkTiming();
    std::set<int> data = ConstructRandomSet(xrange);
    StartBenchmarkTiming();
    for (int j = 0; j < yrange; ++j)
      data.insert(rand());
  }
  SetBenchmarkItemsProcessed(total_iters * yrange);
  SetBenchmarkBytesProcessed(total_iters * yrange * sizeof(int));
}
BENCHMARK(BM_SetInsert)->RangePair(1<<10,8<<10, 1,10);


template<typename Q>
static void BM_Sequential(int iters, int xrange) {
  const int total_iters = iters;
  typename Q::value_type v = 42;
  while (iters-->0) {
    Q q;
    for (int i = xrange; --i; )
      q.push_back(v);
  }
  const int64_t items_processed =
      static_cast<int64_t>(total_iters) * xrange;
  SetBenchmarkItemsProcessed(items_processed);
  SetBenchmarkBytesProcessed(items_processed * sizeof(v));
}
BENCHMARK_TEMPLATE(BM_Sequential, std::vector<int>)->Range(1 << 0, 1 << 10);
BENCHMARK_TEMPLATE(BM_Sequential, std::list<int>)->Range(1 << 0, 1 << 10);


static void BM_StringCompare(int iters, int xrange) {
  StopBenchmarkTiming();
  std::string s1(xrange, '-');
  std::string s2(xrange, '-');
  int r = 0;
  StartBenchmarkTiming();
  while (iters-->0)
    r |= s1.compare(s2);
  // Prevent compiler optimizations
  assert(r != std::numeric_limits<int>::max());
}
BENCHMARK(BM_StringCompare)->Range(1, 1<<20);

static void BM_SetupTeardown_Setup(int) {
  assert(setup_called == false);
  setup_called = true;
  test_vector = new std::vector<int>();
}

static void BM_SetupTeardown_Teardown(int) {
  assert(setup_called);
  setup_called = false;
  delete test_vector;
}

static void BM_SetupTeardown(int iters) {
  int i = 0;
  while (iters-->0) {
    std::lock_guard<std::mutex> l(test_vector_mu);
    if (i%2 == 0)
      test_vector->push_back(i);
    else
      test_vector->pop_back();
    ++i;
  }
}
BENCHMARK(BM_SetupTeardown)->Setup(&BM_SetupTeardown_Setup)
                           ->Teardown(&BM_SetupTeardown_Teardown)
                           ->ThreadPerCpu();


static void BM_LongTest(int iters, int xrange) {
  double tracker = 0.0;
  while (iters-->0) {
    for (int i = 0; i < xrange; ++i)
      tracker += i;
  }
  assert(tracker != 0.0);
}
BENCHMARK(BM_LongTest)->Range(1<<16,1<<28);

int main(int argc, const char* argv[]) {
  benchmark::Initialize(argc, argv);

#ifdef DEBUG
  assert(Factorial(8) == 40320);
#endif
  assert(CalculatePi(1) == 0.0);

  benchmark::RunSpecifiedBenchmarks();
}

