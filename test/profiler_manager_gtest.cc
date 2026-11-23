#include <memory>
#include <vector>

#include "benchmark/benchmark_api.h"
#include "benchmark/managers.h"
#include "benchmark/registration.h"
#include "benchmark/reporter.h"
#include "benchmark/state.h"
#include "benchmark/utils.h"
#include "gtest/gtest.h"

namespace {

class TestProfilerManager : public benchmark::ProfilerManager {
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

void BM_empty(benchmark::State& state) {
  for (auto _ : state) {
    auto iterations = state.iterations();
    benchmark::DoNotOptimize(iterations);
  }
}
BENCHMARK(BM_empty);

int skip_before_loop_runs = 0;

void BM_skip_before_loop(benchmark::State& state) {
  ++skip_before_loop_runs;
  state.SkipWithError("skip before loop");
  for (auto _ : state) {
  }
}
BENCHMARK(BM_skip_before_loop)->Iterations(1);

TEST(ProfilerManager, ReregisterManager) {
#if GTEST_HAS_DEATH_TEST
  // Tests only runnable in debug mode (when BM_CHECK is enabled).
#ifndef NDEBUG
#ifndef TEST_BENCHMARK_LIBRARY_HAS_NO_ASSERTIONS
  ASSERT_DEATH_IF_SUPPORTED(
      {
        std::unique_ptr<TestProfilerManager> pm(new TestProfilerManager());
        benchmark::RegisterProfilerManager(pm.get());
        benchmark::RegisterProfilerManager(pm.get());
      },
      "RegisterProfilerManager");
#endif
#endif
#endif
}

TEST(ProfilerManager, SkippedBeforeLoopDoesNotRunProfilerManager) {
  skip_before_loop_runs = 0;
  TestProfilerManager pm;
  benchmark::RegisterProfilerManager(&pm);
  NullReporter reporter;
  const size_t ran =
      benchmark::RunSpecifiedBenchmarks(&reporter, "^BM_skip_before_loop/");
  benchmark::RegisterProfilerManager(nullptr);

  EXPECT_EQ(ran, 1u);
  EXPECT_EQ(skip_before_loop_runs, 1);
  EXPECT_EQ(pm.start_called, 0);
  EXPECT_EQ(pm.stop_called, 0);
}

}  // namespace
