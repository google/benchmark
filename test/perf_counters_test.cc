#include <cstdarg>
#undef NDEBUG

#include "../src/commandlineflags.h"
#include "../src/perf_counters.h"
#include "benchmark/benchmark.h"
#include "output_test.h"

namespace benchmark {

BM_DECLARE_string(benchmark_perf_counters);

}  // namespace benchmark

static void BM_Simple(benchmark::State& state) {
  for (auto _ : state) {
    auto iterations = state.iterations();
    benchmark::DoNotOptimize(iterations);
  }
}
BENCHMARK(BM_Simple);
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Simple\",$"}});

const int kIters = 1000000;

void BM_WithoutPauseResume(benchmark::State& state) {
  int n = 0;

  for (auto _ : state) {
    for (auto i = 0; i < kIters; ++i) {
      n = 1 - n;
      benchmark::DoNotOptimize(n);
    }
  }
}

BENCHMARK(BM_WithoutPauseResume);
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_WithoutPauseResume\",$"}});

void BM_WithPauseResume(benchmark::State& state) {
  int m = 0, n = 0;

  for (auto _ : state) {
    for (auto i = 0; i < kIters; ++i) {
      n = 1 - n;
      benchmark::DoNotOptimize(n);
    }

    state.PauseTiming();
    for (auto j = 0; j < kIters; ++j) {
      m = 1 - m;
      benchmark::DoNotOptimize(m);
    }
    state.ResumeTiming();
  }
}

BENCHMARK(BM_WithPauseResume);

ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_WithPauseResume\",$"}});

static void CheckSimple(Results const& e) {
  CHECK_COUNTER_VALUE(e, double, "CYCLES", GT, 0);
  CHECK_COUNTER_VALUE(e, double, "BRANCHES", GT, 0.0);
}

static void CheckInstrCount(Results const& e) {
  CHECK_COUNTER_VALUE(e, double, "INSTRUCTIONS", GT, kIters);
}

CHECK_BENCHMARK_RESULTS("BM_Simple", &CheckSimple);
CHECK_BENCHMARK_RESULTS("BM_WithoutPauseResume", &CheckInstrCount);
CHECK_BENCHMARK_RESULTS("BM_WithPauseResume", &CheckInstrCount);

int main(int argc, char* argv[]) {
  if (!benchmark::internal::PerfCounters::kSupported) {
    return 0;
  }
  benchmark::FLAGS_benchmark_perf_counters = "CYCLES,BRANCHES,INSTRUCTIONS";
  benchmark::internal::PerfCounters::Initialize();
  RunOutputTests(argc, argv);
}
