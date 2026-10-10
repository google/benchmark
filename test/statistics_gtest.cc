//===---------------------------------------------------------------------===//
// statistics_test - Unit tests for src/statistics.cc
//===---------------------------------------------------------------------===//

#include <cmath>

#include "../src/complexity.h"
#include "../src/statistics.h"
#include "gtest/gtest.h"

namespace {
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

TEST(ComplexityTest, ComputeBigOFiltersSkippedRuns) {
  benchmark::BenchmarkReporter::Run run1;
  run1.skipped = benchmark::internal::NotSkipped;
  run1.iterations = 1000;
  run1.complexity_n = 10;
  run1.real_accumulated_time = 0.01;
  run1.cpu_accumulated_time = 0.01;
  run1.complexity = benchmark::oN;
  run1.time_unit = benchmark::kNanosecond;

  benchmark::BenchmarkReporter::Run run2;
  run2.skipped = benchmark::internal::SkippedWithError;
  run2.iterations = 0;
  run2.complexity_n = 20;
  run2.real_accumulated_time = 0.0;
  run2.cpu_accumulated_time = 0.0;
  run2.complexity = benchmark::oN;
  run2.time_unit = benchmark::kNanosecond;

  // Single successful run among reports should not emit BigO aggregate
  auto results = benchmark::ComputeBigO({run1, run2});
  EXPECT_TRUE(results.empty());

  benchmark::BenchmarkReporter::Run run3;
  run3.skipped = benchmark::internal::NotSkipped;
  run3.iterations = 2000;
  run3.complexity_n = 20;
  run3.real_accumulated_time = 0.02;
  run3.cpu_accumulated_time = 0.02;
  run3.complexity = benchmark::oN;
  run3.time_unit = benchmark::kNanosecond;

  // Skipped runs should be ignored and not introduce NaN into calculations
  results = benchmark::ComputeBigO({run2, run1, run3});
  ASSERT_EQ(results.size(), 2u);
  EXPECT_EQ(results[0].aggregate_name, "BigO");
  EXPECT_FALSE(std::isnan(results[0].real_accumulated_time));
  EXPECT_FALSE(std::isnan(results[0].cpu_accumulated_time));
  EXPECT_EQ(results[1].aggregate_name, "RMS");
  EXPECT_FALSE(std::isnan(results[1].real_accumulated_time));
  EXPECT_FALSE(std::isnan(results[1].cpu_accumulated_time));
}

}  // end namespace

