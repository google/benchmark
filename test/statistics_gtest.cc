//===---------------------------------------------------------------------===//
// statistics_test - Unit tests for src/statistics.cc
//===---------------------------------------------------------------------===//

#include "../src/statistics.h"
#include "gtest/gtest.h"

namespace {
using BenchmarkRun = benchmark::BenchmarkReporter::Run;

BenchmarkRun MakeRun(
    const std::vector<benchmark::internal::Statistics>* statistics,
    int64_t repetition_index, double real_time, double cpu_time) {
  BenchmarkRun run;
  run.run_name.function_name = "BM_Flaky";
  run.family_index = 1;
  run.per_family_instance_index = 2;
  run.iterations = 10;
  run.repetition_index = repetition_index;
  run.repetitions = 3;
  run.time_unit = benchmark::kSecond;
  run.real_accumulated_time = real_time;
  run.cpu_accumulated_time = cpu_time;
  run.statistics = statistics;
  return run;
}

TEST(StatisticsTest, Mean) {
  EXPECT_DOUBLE_EQ(benchmark::StatisticsMean({42, 42, 42, 42}), 42.0);
  EXPECT_DOUBLE_EQ(benchmark::StatisticsMean({1, 2, 3, 4}), 2.5);
  EXPECT_DOUBLE_EQ(benchmark::StatisticsMean({1, 2, 5, 10, 10, 14}), 7.0);
}

TEST(StatisticsTest, Median) {
  EXPECT_DOUBLE_EQ(benchmark::StatisticsMedian({42, 42, 42, 42}), 42.0);
  EXPECT_DOUBLE_EQ(benchmark::StatisticsMedian({1, 2, 3, 4}), 2.5);
  EXPECT_DOUBLE_EQ(benchmark::StatisticsMedian({1, 2, 5, 10, 10}), 5.0);
}

TEST(StatisticsTest, StdDev) {
  EXPECT_DOUBLE_EQ(benchmark::StatisticsStdDev({101, 101, 101, 101}), 0.0);
  EXPECT_DOUBLE_EQ(benchmark::StatisticsStdDev({1, 2, 3}), 1.0);
  EXPECT_DOUBLE_EQ(benchmark::StatisticsStdDev({2.5, 2.4, 3.3, 4.2, 5.1}),
                   1.151086443322134);
}

TEST(StatisticsTest, CV) {
  EXPECT_DOUBLE_EQ(benchmark::StatisticsCV({101, 101, 101, 101}), 0.0);
  EXPECT_DOUBLE_EQ(benchmark::StatisticsCV({1, 2, 3}), 1. / 2.);
  ASSERT_NEAR(benchmark::StatisticsCV({2.5, 2.4, 3.3, 4.2, 5.1}),
              0.32888184094918121, 1e-15);
}

TEST(StatisticsTest, ComputeStatsSkipsErroredFirstRun) {
  const std::vector<benchmark::internal::Statistics> statistics = {
      {"mean", benchmark::StatisticsMean}};

  BenchmarkRun skipped = MakeRun(nullptr, 0, 0.0, 0.0);
  skipped.skipped = benchmark::internal::SkippedWithError;

  const std::vector<BenchmarkRun> reports = {
      skipped,
      MakeRun(&statistics, 1, 20.0, 30.0),
      MakeRun(&statistics, 2, 40.0, 50.0),
  };

  const std::vector<BenchmarkRun> results = benchmark::ComputeStats(reports);

  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].benchmark_name(), "BM_Flaky_mean");
  EXPECT_EQ(results[0].iterations, 2);
  EXPECT_DOUBLE_EQ(results[0].GetAdjustedRealTime(), 3.0);
  EXPECT_DOUBLE_EQ(results[0].GetAdjustedCPUTime(), 4.0);
}

}  // end namespace
