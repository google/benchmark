// Testing:
//   State::PauseTiming()
//   State::ResumeTiming()
// Test that the assertions in these functions diagnose calls made outside of
// the benchmark loop, and that a run they do not diagnose still reports a
// sane time.

#include <string>
#include <vector>

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace {

void BM_pause_before_loop(benchmark::State& state) {
  state.PauseTiming();
  for (auto _ : state) {
  }
}
BENCHMARK(BM_pause_before_loop)->Iterations(1);

void BM_resume_before_loop(benchmark::State& state) {
  state.ResumeTiming();
  for (auto _ : state) {
  }
}
BENCHMARK(BM_resume_before_loop)->Iterations(1);

void BM_pause_after_loop(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.PauseTiming();
}
BENCHMARK(BM_pause_after_loop)->Iterations(1);

void BM_resume_after_loop(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.ResumeTiming();
}
BENCHMARK(BM_resume_after_loop)->Iterations(1);

void BM_pause_and_resume_in_loop(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    state.ResumeTiming();
  }
}
BENCHMARK(BM_pause_and_resume_in_loop)->Iterations(1);

class CapturingReporter : public benchmark::BenchmarkReporter {
 public:
  bool ReportContext(const Context& /*context*/) override { return true; }
  void ReportRuns(const std::vector<Run>& runs) override {
    runs_.insert(runs_.end(), runs.begin(), runs.end());
  }

  const std::vector<Run>& runs() const { return runs_; }

 private:
  std::vector<Run> runs_;
};

std::vector<benchmark::BenchmarkReporter::Run> RunOne(const std::string& name) {
  CapturingReporter reporter;
  benchmark::RunSpecifiedBenchmarks(&reporter, name);
  return reporter.runs();
}

TEST(Diagnostics, PauseOutsideOfTheLoopIsDiagnosed) {
#ifndef NDEBUG
  ASSERT_DEATH_IF_SUPPORTED(RunOne("BM_pause_before_loop"), "PauseTiming");
  ASSERT_DEATH_IF_SUPPORTED(RunOne("BM_pause_after_loop"), "PauseTiming");
#endif
}

TEST(Diagnostics, ResumeOutsideOfTheLoopIsDiagnosed) {
#ifndef NDEBUG
  ASSERT_DEATH_IF_SUPPORTED(RunOne("BM_resume_before_loop"), "ResumeTiming");
  ASSERT_DEATH_IF_SUPPORTED(RunOne("BM_resume_after_loop"), "ResumeTiming");
#endif
}

TEST(Diagnostics, PauseAndResumeInsideTheLoopReportASaneTime) {
  const std::vector<benchmark::BenchmarkReporter::Run> runs =
      RunOne("BM_pause_and_resume_in_loop");
  ASSERT_EQ(runs.size(), 1u);
  EXPECT_EQ(runs[0].skipped, 0u);
  // One iteration of an empty loop. A whole second would mean an absolute
  // clock reading was accumulated instead of a duration.
  EXPECT_GE(runs[0].real_accumulated_time, 0.0);
  EXPECT_LT(runs[0].real_accumulated_time, 1.0);
  EXPECT_GE(runs[0].cpu_accumulated_time, 0.0);
  EXPECT_LT(runs[0].cpu_accumulated_time, 1.0);
}

}  // namespace
