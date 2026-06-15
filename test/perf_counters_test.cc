#include <cstdarg>
#include <set>
#include <string>
#include <vector>
#undef NDEBUG

#include "../src/commandlineflags.h"
#include "../src/perf_counters.h"
#include "benchmark/benchmark_api.h"
#include "benchmark/registration.h"
#include "benchmark/state.h"
#include "benchmark/utils.h"
#include "output_test.h"

namespace benchmark {

BM_DECLARE_string(benchmark_perf_counters);

}  // namespace benchmark
namespace {
const char kGenericPerfEvent1[] = "CYCLES";
const char kGenericPerfEvent2[] = "INSTRUCTIONS";

std::set<std::string> UniqueCounterNames(
    const benchmark::internal::PerfCounters& counters) {
  return {counters.names().begin(), counters.names().end()};
}

bool HasRequiredPerfCounters(const std::vector<std::string>& names) {
  if (!benchmark::internal::PerfCounters::kSupported) {
    return false;
  }
  auto counters = benchmark::internal::PerfCounters::Create(names);
  auto actual_names = UniqueCounterNames(counters);
  for (const auto& name : names) {
    if (actual_names.find(name) == actual_names.end()) {
      return false;
    }
  }
  return true;
}

void BM_Simple(benchmark::State& state) {
  for (auto _ : state) {
    auto iterations = double(state.iterations()) * double(state.iterations());
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
  CHECK_COUNTER_VALUE(e, double, kGenericPerfEvent1, GT, 0);
}

double withoutPauseResumeInstrCount = 0.0;
double withPauseResumeInstrCount = 0.0;

void SaveInstrCountWithoutResume(Results const& e) {
  withoutPauseResumeInstrCount = e.GetAs<double>(kGenericPerfEvent2);
}

void SaveInstrCountWithResume(Results const& e) {
  withPauseResumeInstrCount = e.GetAs<double>(kGenericPerfEvent2);
}

CHECK_BENCHMARK_RESULTS("BM_Simple", &CheckSimple);
CHECK_BENCHMARK_RESULTS("BM_WithoutPauseResume", &SaveInstrCountWithoutResume);
CHECK_BENCHMARK_RESULTS("BM_WithPauseResume", &SaveInstrCountWithResume);
}  // end namespace

int main(int argc, char* argv[]) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  if (!HasRequiredPerfCounters({kGenericPerfEvent1, kGenericPerfEvent2})) {
    return 0;
  }
  benchmark::FLAGS_benchmark_perf_counters =
      std::string(kGenericPerfEvent1) + "," + kGenericPerfEvent2;
  benchmark::internal::PerfCounters::Initialize();
  RunOutputTests(argc, argv);

  BM_CHECK_GT(withPauseResumeInstrCount, kIters);
  BM_CHECK_GT(withoutPauseResumeInstrCount, kIters);
  BM_CHECK_LT(withPauseResumeInstrCount, 1.5 * withoutPauseResumeInstrCount);
}
