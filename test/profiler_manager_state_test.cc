#include <cassert>

#include <string>
#include <vector>

#include "benchmark/benchmark_api.h"
#include "benchmark/managers.h"
#include "benchmark/registration.h"
#include "benchmark/reporter.h"
#include "benchmark/state.h"
#include "benchmark/utils.h"

namespace {

class StateProfilerManager : public benchmark::ProfilerManager {
 public:
  void AfterSetupStartWithState(const benchmark::State& state) override {
    ++start_called;
    start_name = state.name();
  }

  void BeforeTeardownStopWithState(const benchmark::State& state) override {
    ++stop_called;
    stop_name = state.name();
  }

  int start_called = 0;
  int stop_called = 0;
  std::string start_name;
  std::string stop_name;
};

class LegacyProfilerManager : public benchmark::ProfilerManager {
 public:
  void AfterSetupStart() override { ++start_called; }
  void BeforeTeardownStop() override { ++stop_called; }

  int start_called = 0;
  int stop_called = 0;
};

class NullReporter : public benchmark::BenchmarkReporter {
 public:
  bool ReportContext(const Context& /*context*/) override { return true; }
  void ReportRuns(const std::vector<Run>& /*report*/) override {}
};

void BM_Profiled(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(state.iterations());
  }
}
BENCHMARK(BM_Profiled);

}  // namespace

int main(int argc, char* argv[]) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  benchmark::Initialize(&argc, argv);

  NullReporter null_reporter;

  StateProfilerManager state_profiler;
  benchmark::RegisterProfilerManager(&state_profiler);
  size_t run_count = benchmark::RunSpecifiedBenchmarks(&null_reporter,
                                                       "BM_Profiled$");
  benchmark::RegisterProfilerManager(nullptr);

  assert(run_count == 1);
  assert(state_profiler.start_called == 1);
  assert(state_profiler.stop_called == 1);
  assert(state_profiler.start_name == "BM_Profiled");
  assert(state_profiler.stop_name == "BM_Profiled");

  LegacyProfilerManager legacy_profiler;
  benchmark::RegisterProfilerManager(&legacy_profiler);
  run_count = benchmark::RunSpecifiedBenchmarks(&null_reporter, "BM_Profiled$");
  benchmark::RegisterProfilerManager(nullptr);

  assert(run_count == 1);
  assert(legacy_profiler.start_called == 1);
  assert(legacy_profiler.stop_called == 1);

  return 0;
}
