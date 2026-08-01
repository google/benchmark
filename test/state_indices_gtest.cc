// Checks that a running benchmark can tell which run it is: the family it was
// registered from, which instance of that family it is, and which repetition
// is executing. See https://github.com/google/benchmark/issues/1677.

#include <string>
#include <tuple>
#include <vector>

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace benchmark {
namespace {

// family index, per-family instance index, repetition index, repetitions.
using Indices = std::tuple<int, int, int, int>;

Indices IndicesOf(const State& state) {
  return {state.family_index(), state.per_family_instance_index(),
          state.repetition_index(), state.repetitions()};
}

// Everything below runs single-threaded on the main thread, so the recorded
// values need no synchronization.
std::vector<Indices> from_benchmark;
std::vector<Indices> from_setup;
std::vector<Indices> from_teardown;
std::vector<Indices> from_report;

void DoSetup(const State& state) { from_setup.push_back(IndicesOf(state)); }
void DoTeardown(const State& state) {
  from_teardown.push_back(IndicesOf(state));
}

void BM_first(State& state) {
  from_benchmark.push_back(IndicesOf(state));
  for (auto _ : state) {
  }
}
BENCHMARK(BM_first)
    ->Arg(1)
    ->Arg(2)
    ->Iterations(1)
    ->Repetitions(2)
    ->Setup(DoSetup)
    ->Teardown(DoTeardown);

void BM_second(State& state) {
  from_benchmark.push_back(IndicesOf(state));
  for (auto _ : state) {
  }
}
BENCHMARK(BM_second)->Iterations(1)->Repetitions(2);

// Collects the indices the library itself reports, so the values observed from
// inside the benchmark can be checked against them.
class IndexCollectingReporter : public BenchmarkReporter {
 public:
  bool ReportContext(const Context&) override { return true; }
  void ReportRuns(const std::vector<Run>& runs) override {
    for (const Run& run : runs) {
      // Aggregates describe a set of repetitions, not a single run.
      if (run.run_type != Run::RT_Iteration) {
        continue;
      }
      from_report.emplace_back(run.family_index, run.per_family_instance_index,
                               run.repetition_index, run.repetitions);
    }
  }
};

}  // namespace

TEST(StateIndices, MatchTheReportedRun) {
  IndexCollectingReporter reporter;
  ASSERT_GT(RunSpecifiedBenchmarks(&reporter), 0u);

  // Two instances of BM_first and one of BM_second, two repetitions each.
  const std::vector<Indices> expected = {
      {0, 0, 0, 2}, {0, 0, 1, 2},  // BM_first/1
      {0, 1, 0, 2}, {0, 1, 1, 2},  // BM_first/2
      {1, 0, 0, 2}, {1, 0, 1, 2},  // BM_second
  };

  EXPECT_EQ(from_benchmark, expected);
  EXPECT_EQ(from_report, expected);

  // Setup and teardown callbacks run once per repetition and see the same
  // indices as the run they bracket.
  const std::vector<Indices> expected_first(expected.begin(),
                                            expected.begin() + 4);
  EXPECT_EQ(from_setup, expected_first);
  EXPECT_EQ(from_teardown, expected_first);
}

}  // namespace benchmark
