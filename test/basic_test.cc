
#include "benchmark/benchmark.h"

using benchmark::StartBenchmarkTiming;
using benchmark::StopBenchmarkTiming;

#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)->Arg(8)->Arg(512)->Arg(8192)


void BM_empty(int iters) {
  while (iters-- > 0) {
    volatile int x = iters;
    ((void)x);
  }
}
BENCHMARK(BM_empty);
BENCHMARK(BM_empty)->ThreadPerCpu();

void BM_spin(int iters, int xrange) {
  while (iters-- > 0) {
    for (int x = 0; x < xrange; ++x) {
      volatile int dummy = x;
      ((void)dummy);
    }
  }
}
BASIC_BENCHMARK_TEST(BM_spin);
BASIC_BENCHMARK_TEST(BM_spin)->ThreadPerCpu();

void BM_spin_pause_before(int iters, int xrange) {
  StopBenchmarkTiming();
  for (int i = 0; i < xrange; ++i) {
    volatile int dummy = i;
    ((void)dummy);
  }
  StartBenchmarkTiming();
  while(iters-- > 0) {
    for (int i = 0; i < xrange; ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_before);
BASIC_BENCHMARK_TEST(BM_spin_pause_before)->ThreadPerCpu();

void BM_spin_pause_during(int iters, int xrange) {
  while(iters-- > 0) {
    StopBenchmarkTiming();
    for (int i = 0; i < xrange; ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
    StartBenchmarkTiming();
    for (int i = 0; i < xrange; ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_during);
BASIC_BENCHMARK_TEST(BM_spin_pause_during)->ThreadPerCpu();


void BM_spin_pause_after(int iters, int xrange) {
  while(iters-- > 0) {
    for (int i = 0; i < xrange; ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
  }
  StopBenchmarkTiming();
  for (int i = 0; i < xrange; ++i) {
    volatile int dummy = i;
    ((void)dummy);
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_after);
BASIC_BENCHMARK_TEST(BM_spin_pause_after)->ThreadPerCpu();

void BM_spin_pause_before_and_after(int iters, int xrange) {
  StopBenchmarkTiming();
  for (int i = 0; i < xrange; ++i) {
    volatile int dummy = i;
    ((void)dummy);
  }
  StartBenchmarkTiming();
  while(iters-- > 0) {
    for (int i = 0; i < xrange; ++i) {
      volatile int dummy = i;
      ((void)dummy);
    }
  }
  StopBenchmarkTiming();
  for (int i = 0; i < xrange; ++i) {
    volatile int dummy = i;
    ((void)dummy);
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_before_and_after);
BASIC_BENCHMARK_TEST(BM_spin_pause_before_and_after)->ThreadPerCpu();


void BM_empty_stop_start(int iters) {
  StopBenchmarkTiming();
  StartBenchmarkTiming();
  while (iters-- > 0) { }
}
BENCHMARK(BM_empty_stop_start);
BENCHMARK(BM_empty_stop_start)->ThreadPerCpu();

BENCHMARK_MAIN()
