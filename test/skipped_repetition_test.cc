
#undef NDEBUG
#include <cassert>
#include <vector>

#include "../src/check.h"  // NOTE: check.h is for internal use only!
#include "benchmark/benchmark_api.h"
#include "benchmark/registration.h"
#include "benchmark/reporter.h"
#include "benchmark/state.h"
#include "benchmark/utils.h"

namespace {

class TestReporter : public benchmark::ConsoleReporter {
 public:
  bool ReportContext(const Context& context) override {
    return ConsoleReporter::ReportContext(context);
  };

  void ReportRuns(const std::vector<Run>& report) override {
    all_runs_.insert(all_runs_.end(), begin(report), end(report));
    ConsoleReporter::ReportRuns(report);
  }

  TestReporter() {}
  ~TestReporter() override {}

  mutable std::vector<Run> all_runs_;
};

// Regression test for a crash in ComputeStats() when aggregating across
// repetitions where the first repetition is skipped.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int invocation_count = 0;

void BM_skip_first_rep(benchmark::State& state) {
  if (invocation_count++ == 0) {
    state.SkipWithError("first repetition skipped");
    return;
  }
  for (auto _ : state) {
    benchmark::DoNotOptimize(invocation_count);
  }
}
BENCHMARK(BM_skip_first_rep)->Repetitions(3);

}  // end namespace

int main(int argc, char* argv[]) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  benchmark::Initialize(&argc, argv);

  TestReporter test_reporter;
  benchmark::RunSpecifiedBenchmarks(&test_reporter);

  typedef benchmark::BenchmarkReporter::Run Run;

  int iteration_runs = 0;
  int skipped_iteration_runs = 0;
  int aggregate_runs = 0;
  for (Run const& run : test_reporter.all_runs_) {
    if (run.run_type == Run::RT_Aggregate) {
      ++aggregate_runs;
    } else {
      ++iteration_runs;
      if (run.skipped == benchmark::internal::SkippedWithError) {
        ++skipped_iteration_runs;
      }
    }
  }

  BM_CHECK(iteration_runs == 3);
  BM_CHECK(skipped_iteration_runs == 1);
  // Default statistics: mean, median, stddev, cv.
  BM_CHECK(aggregate_runs == 4);

  return 0;
}
