#include <chrono>
#include <thread>
#include <vector>

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

// Regression test for #2117: the --benchmark_min_time budget must be respected
// *per thread*. Because real/manual time (and whole-process CPU time) are
// accumulated across all threads, the min-time stopping decision has to divide
// that accumulated time by the thread count. Otherwise a benchmark that uses
// N threads stops after only min_time/N of wall-clock time per thread.

namespace {

using benchmark::Benchmark;
using benchmark::ClearRegisteredBenchmarks;
using benchmark::ConsoleReporter;
using benchmark::RegisterBenchmark;
using benchmark::RunSpecifiedBenchmarks;
using benchmark::State;

constexpr double kMinTime = 0.1;  // seconds, matches the flag set below.

void BM_ThreadedSleep(State& state) {
  for (auto _ : state) {
    std::this_thread::sleep_for(std::chrono::microseconds(200));
  }
}

class TestReporter : public ConsoleReporter {
 public:
  bool ReportContext(const Context& /*unused*/) override { return true; }
  void PrintHeader(const Run&) override {}
  void PrintRunData(const Run& run) override {
    // Ignore aggregate rows (mean/median/stddev), keep the real runs.
    if (run.repetition_index < 0) return;
    runs.push_back(run);
  }

  // Wall-clock time a single thread spent in the run with `threads` threads.
  double PerThreadWallTime(int threads) const {
    for (const auto& run : runs) {
      if (run.threads == threads) {
        // real_accumulated_time is summed across all threads.
        return run.real_accumulated_time / static_cast<double>(run.threads);
      }
    }
    return -1.0;
  }

  std::vector<Run> runs;
};

}  // namespace

TEST(MinTimeThreadsTest, MinTimeRespectedPerThread) {
  RegisterBenchmark("BM_ThreadedSleep", BM_ThreadedSleep)
      ->UseRealTime()
      ->Threads(1)
      ->Threads(4);

  const char* argv[] = {"min_time_threads_gtest", "--benchmark_min_time=0.1s"};
  int argc = 2;
  benchmark::Initialize(&argc, const_cast<char**>(argv));

  TestReporter reporter;
  RunSpecifiedBenchmarks(&reporter, "BM_ThreadedSleep");
  ClearRegisteredBenchmarks();

  const double wall_1 = reporter.PerThreadWallTime(1);
  const double wall_4 = reporter.PerThreadWallTime(4);
  ASSERT_GT(wall_1, 0.0) << "single-threaded run missing";
  ASSERT_GT(wall_4, 0.0) << "4-threaded run missing";

  // Each thread should run for roughly min_time, independent of thread count.
  EXPECT_GE(wall_1, 0.7 * kMinTime);
  // The core assertion: with the bug the 4-thread run stops after ~min_time/4
  // per thread, so this would be ~0.25 * wall_1 instead of ~wall_1.
  EXPECT_GE(wall_4, 0.7 * wall_1)
      << "per-thread wall time collapsed with more threads: 1-thread=" << wall_1
      << "s 4-thread=" << wall_4 << "s";
}
