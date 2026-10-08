#include <map>
#include <string>
#include <thread>
#include <vector>

#include "../src/benchmark_register.h"
#include "../src/mutex.h"
#include "benchmark/benchmark.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace benchmark {
namespace internal {

namespace {

TEST(AddRangeTest, Simple) {
  std::vector<int> dst;
  AddRange(&dst, 1, 2, 2);
  EXPECT_THAT(dst, testing::ElementsAre(1, 2));
}

TEST(AddRangeTest, Simple64) {
  std::vector<int64_t> dst;
  AddRange(&dst, static_cast<int64_t>(1), static_cast<int64_t>(2), 2);
  EXPECT_THAT(dst, testing::ElementsAre(1, 2));
}

TEST(AddRangeTest, Advanced) {
  std::vector<int> dst;
  AddRange(&dst, 5, 15, 2);
  EXPECT_THAT(dst, testing::ElementsAre(5, 8, 15));
}

TEST(AddRangeTest, Advanced64) {
  std::vector<int64_t> dst;
  AddRange(&dst, static_cast<int64_t>(5), static_cast<int64_t>(15), 2);
  EXPECT_THAT(dst, testing::ElementsAre(5, 8, 15));
}

TEST(AddRangeTest, FullRange8) {
  std::vector<int8_t> dst;
  AddRange(&dst, int8_t{1}, std::numeric_limits<int8_t>::max(), 8);
  EXPECT_THAT(
      dst, testing::ElementsAre(int8_t{1}, int8_t{8}, int8_t{64}, int8_t{127}));
}

TEST(AddRangeTest, FullRange64) {
  std::vector<int64_t> dst;
  AddRange(&dst, int64_t{1}, std::numeric_limits<int64_t>::max(), 1024);
  EXPECT_THAT(
      dst, testing::ElementsAre(1LL, 1024LL, 1048576LL, 1073741824LL,
                                1099511627776LL, 1125899906842624LL,
                                1152921504606846976LL, 9223372036854775807LL));
}

TEST(AddRangeTest, NegativeRanges) {
  std::vector<int> dst;
  AddRange(&dst, -8, 0, 2);
  EXPECT_THAT(dst, testing::ElementsAre(-8, -4, -2, -1, 0));
}

TEST(AddRangeTest, StrictlyNegative) {
  std::vector<int> dst;
  AddRange(&dst, -8, -1, 2);
  EXPECT_THAT(dst, testing::ElementsAre(-8, -4, -2, -1));
}

TEST(AddRangeTest, SymmetricNegativeRanges) {
  std::vector<int> dst;
  AddRange(&dst, -8, 8, 2);
  EXPECT_THAT(dst, testing::ElementsAre(-8, -4, -2, -1, 0, 1, 2, 4, 8));
}

TEST(AddRangeTest, SymmetricNegativeRangesOddMult) {
  std::vector<int> dst;
  AddRange(&dst, -30, 32, 5);
  EXPECT_THAT(dst, testing::ElementsAre(-30, -25, -5, -1, 0, 1, 5, 25, 32));
}

TEST(AddRangeTest, NegativeRangesAsymmetric) {
  std::vector<int> dst;
  AddRange(&dst, -3, 5, 2);
  EXPECT_THAT(dst, testing::ElementsAre(-3, -2, -1, 0, 1, 2, 4, 5));
}

TEST(AddRangeTest, NegativeRangesLargeStep) {
  // Always include -1, 0, 1 when crossing zero.
  std::vector<int> dst;
  AddRange(&dst, -8, 8, 10);
  EXPECT_THAT(dst, testing::ElementsAre(-8, -1, 0, 1, 8));
}

TEST(AddRangeTest, ZeroOnlyRange) {
  std::vector<int> dst;
  AddRange(&dst, 0, 0, 2);
  EXPECT_THAT(dst, testing::ElementsAre(0));
}

TEST(AddRangeTest, ZeroStartingRange) {
  std::vector<int> dst;
  AddRange(&dst, 0, 2, 2);
  EXPECT_THAT(dst, testing::ElementsAre(0, 1, 2));
}

TEST(AddRangeTest, NegativeRange64) {
  std::vector<int64_t> dst;
  AddRange<int64_t>(&dst, -4, 4, 2);
  EXPECT_THAT(dst, testing::ElementsAre(-4, -2, -1, 0, 1, 2, 4));
}

TEST(AddRangeTest, NegativeRangePreservesExistingOrder) {
  // If elements already exist in the range, ensure we don't change
  // their ordering by adding negative values.
  std::vector<int64_t> dst = {1, 2, 3};
  AddRange<int64_t>(&dst, -2, 2, 2);
  EXPECT_THAT(dst, testing::ElementsAre(1, 2, 3, -2, -1, 0, 1, 2));
}

TEST(AddRangeTest, FullNegativeRange64) {
  std::vector<int64_t> dst;
  const auto min = std::numeric_limits<int64_t>::min();
  const auto max = std::numeric_limits<int64_t>::max();
  AddRange(&dst, min, max, 1024);
  EXPECT_THAT(
      dst, testing::ElementsAreArray(std::vector<int64_t>{
               min, -1152921504606846976LL, -1125899906842624LL,
               -1099511627776LL, -1073741824LL, -1048576LL, -1024LL, -1LL, 0LL,
               1LL, 1024LL, 1048576LL, 1073741824LL, 1099511627776LL,
               1125899906842624LL, 1152921504606846976LL, max}));
}

TEST(AddRangeTest, Simple8) {
  std::vector<int8_t> dst;
  AddRange<int8_t>(&dst, int8_t{1}, int8_t{8}, int8_t{2});
  EXPECT_THAT(dst,
              testing::ElementsAre(int8_t{1}, int8_t{2}, int8_t{4}, int8_t{8}));
}

TEST(AddCustomContext, Simple) {
  std::map<std::string, std::string>*& global_context = GetGlobalContext();
  EXPECT_THAT(global_context, nullptr);

  AddCustomContext("foo", "bar");
  AddCustomContext("baz", "qux");

  EXPECT_THAT(*global_context,
              testing::UnorderedElementsAre(testing::Pair("foo", "bar"),
                                            testing::Pair("baz", "qux")));

  delete global_context;
  global_context = nullptr;
}

TEST(AddCustomContext, DuplicateKey) {
  std::map<std::string, std::string>*& global_context = GetGlobalContext();
  EXPECT_THAT(global_context, nullptr);

  AddCustomContext("foo", "bar");
  AddCustomContext("foo", "qux");

  EXPECT_THAT(*global_context,
              testing::UnorderedElementsAre(testing::Pair("foo", "bar")));

  delete global_context;
  global_context = nullptr;
}

// PauseTiming() and ResumeTiming() assert when called outside the benchmark
// loop.
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

class CapturingReporter : public BenchmarkReporter {
 public:
  bool ReportContext(const Context& /*context*/) override { return true; }
  void ReportRuns(const std::vector<Run>& runs) override {
    runs_.insert(runs_.end(), runs.begin(), runs.end());
  }

  const std::vector<Run>& runs() const { return runs_; }

 private:
  std::vector<Run> runs_;
};

std::vector<BenchmarkReporter::Run> RunOne(const std::string& name) {
  CapturingReporter reporter;
  RunSpecifiedBenchmarks(&reporter, name);
  return reporter.runs();
}

TEST(TimingDeathTest, PauseBeforeLoop) {
  ASSERT_DEBUG_DEATH(RunOne("BM_pause_before_loop"), "PauseTiming");
}

TEST(TimingDeathTest, PauseAfterLoop) {
  ASSERT_DEBUG_DEATH(RunOne("BM_pause_after_loop"), "PauseTiming");
}

TEST(TimingDeathTest, ResumeBeforeLoop) {
  ASSERT_DEBUG_DEATH(RunOne("BM_resume_before_loop"), "ResumeTiming");
}

TEST(TimingDeathTest, ResumeAfterLoop) {
  ASSERT_DEBUG_DEATH(RunOne("BM_resume_after_loop"), "ResumeTiming");
}

TEST(TimingTest, PauseAndResumeInLoopReportSaneTime) {
  const std::vector<BenchmarkReporter::Run> runs =
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

TEST(BarrierTest, SingleThreadedWaitAndTeardown) {
  for (int i = 0; i < 100; ++i) {
    auto barrier = std::make_unique<Barrier>(1);
    EXPECT_TRUE(barrier->wait());
  }
}

TEST(BarrierTest, MultiThreadedWaitAndTeardown) {
  constexpr int kNumThreads = 8;
  for (int i = 0; i < 100; ++i) {
    auto barrier = std::make_unique<Barrier>(kNumThreads);
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int t = 0; t < kNumThreads; ++t) {
      threads.emplace_back([&barrier]() {
        barrier->wait();
      });
    }
    for (auto& thread : threads) {
      thread.join();
    }
  }
}

TEST(BarrierTest, MultiThreadedRemoveThreadAndTeardown) {
  constexpr int kNumThreads = 4;
  for (int i = 0; i < 100; ++i) {
    auto barrier = std::make_unique<Barrier>(kNumThreads);
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int t = 0; t < kNumThreads - 1; ++t) {
      threads.emplace_back([&barrier]() {
        barrier->wait();
      });
    }
    threads.emplace_back([&barrier]() {
      barrier->removeThread();
    });
    for (auto& thread : threads) {
      thread.join();
    }
  }
}

}  // namespace
}  // namespace internal
}  // namespace benchmark
