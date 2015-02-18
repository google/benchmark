
#include "benchmark/benchmark.h"

using benchmark::StartBenchmarkTiming;
using benchmark::StopBenchmarkTiming;

void BM_empty(int iters) {
  while (iters-- > 0) { }
}
BENCHMARK(BM_empty);
BENCHMARK(BM_empty)->ThreadPerCpu();

void BM_empty2(int iters) {
  StopBenchmarkTiming();
  StartBenchmarkTiming();
  while (iters-- > 0) { }
}
BENCHMARK(BM_empty2)->ThreadPerCpu();

void BM_spin(int iters, int xrange) {
  while (iters-- > 0) {
    for (int x = 0; x < xrange; ++x) {
      volatile int dummy = x;
      ((void)dummy);
    }
  }
}
BENCHMARK(BM_spin)->Range(8, 8<<10);

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
BENCHMARK(BM_spin_pause_before)->Range(8, 8<<10);
BENCHMARK(BM_spin_pause_before)->Range(8, 8<<10)->ThreadPerCpu();


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
BENCHMARK(BM_spin_pause_during)->Range(8, 8<<10);
BENCHMARK(BM_spin_pause_during)->Range(8, 8<<10)->ThreadPerCpu();

BENCHMARK_MAIN()
