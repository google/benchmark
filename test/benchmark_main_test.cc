#include "benchmark/benchmark.h"

#include <assert.h>

#include <limits>
#include <string>

static void BM_StringCompare(benchmark::State& state) {
  std::string s1(state.range_x(), '-');
  std::string s2(state.range_x(), '-');
  int r = 0;
  while (state.KeepRunning())
    r |= s1.compare(s2);
  // Prevent compiler optimizations
  assert(r != std::numeric_limits<int>::max());
}
BENCHMARK(BM_StringCompare)->Range(1, 1<<20);

