#include <string>
#include <vector>

#include "../src/commandlineflags.h"
#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace benchmark {

BM_DECLARE_bool(benchmark_dry_run);
BM_DECLARE_string(benchmark_min_time);
BM_DECLARE_double(benchmark_min_warmup_time);

namespace {

class CapturingReporter : public BenchmarkReporter {
 public:
  bool ReportContext(const Context&) override { return true; }
  void ReportRuns(const std::vector<Run>& reports) override {
    runs.insert(runs.end(), reports.begin(), reports.end());
  }

  std::vector<Run> runs;
};

class WarmupTest : public testing::Test {
 protected:
  void SetUp() override {
    FLAGS_benchmark_dry_run = false;
    FLAGS_benchmark_min_time = "0.01s";
    FLAGS_benchmark_min_warmup_time = 0.0;
    benchmark_ = RegisterBenchmark("Warmup", [this](State& state) {
      ++invocations_;
      // Manual time makes calibration independent of the clock. The first
      // invocation is deliberately slower so we can detect a reported warmup.
      for (auto _ : state) {
        state.SetIterationTime(invocations_ == 1 ? 2.0 : 1.0);
      }
    });
    benchmark_->UseManualTime()->Repetitions(1);
  }

  void TearDown() override {
    ClearRegisteredBenchmarks();
    FLAGS_benchmark_dry_run = saved_dry_run_;
    FLAGS_benchmark_min_time = saved_min_time_;
    FLAGS_benchmark_min_warmup_time = saved_min_warmup_time_;
  }

  void ExpectRun(int expected_invocations, double expected_time) {
    CapturingReporter reporter;
    ASSERT_EQ(RunSpecifiedBenchmarks(&reporter, "^Warmup/"), 1u);
    EXPECT_EQ(invocations_, expected_invocations);
    ASSERT_EQ(reporter.runs.size(), 1u);
    const auto& run = reporter.runs.front();
    EXPECT_EQ(run.skipped, internal::NotSkipped);
    EXPECT_EQ(run.iterations, 1);
    EXPECT_EQ(run.repetitions, 1);
    EXPECT_DOUBLE_EQ(run.real_accumulated_time, expected_time);
  }

  Benchmark* benchmark_ = nullptr;

 private:
  const bool saved_dry_run_ = FLAGS_benchmark_dry_run;
  const std::string saved_min_time_ = FLAGS_benchmark_min_time;
  const double saved_min_warmup_time_ = FLAGS_benchmark_min_warmup_time;
  int invocations_ = 0;
};

TEST_F(WarmupTest, LocalWarmupWithoutMinTime) {
  benchmark_->MinWarmUpTime(0.01);
  ExpectRun(2, 1.0);
}

TEST_F(WarmupTest, LocalWarmupWithMinTime) {
  benchmark_->MinWarmUpTime(0.01)->MinTime(0.01);
  ExpectRun(2, 1.0);
}

TEST_F(WarmupTest, LocalWarmupOverridesGlobalWithoutMinTime) {
  FLAGS_benchmark_min_warmup_time = 3.0;
  benchmark_->MinWarmUpTime(0.01);
  ExpectRun(2, 1.0);
}

TEST_F(WarmupTest, GlobalWarmupFallback) {
  FLAGS_benchmark_min_warmup_time = 0.01;
  ExpectRun(2, 1.0);
}

TEST_F(WarmupTest, NoWarmup) { ExpectRun(1, 2.0); }

TEST_F(WarmupTest, DryRunSkipsWarmup) {
  FLAGS_benchmark_dry_run = true;
  FLAGS_benchmark_min_warmup_time = 3.0;
  benchmark_->MinWarmUpTime(0.01)->MinTime(3.0)->Repetitions(3);
  ExpectRun(1, 2.0);
}

}  // namespace
}  // namespace benchmark
