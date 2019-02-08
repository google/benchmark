#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace {

using namespace benchmark;
using namespace benchmark::internal;

#define EXPECT_FIELD(name, field, expected) \
  EXPECT_EQ((name.get(BenchmarkName::field)), (expected));

TEST(BenchmarkNameTest, Empty) {
  const auto name = BenchmarkName();

  EXPECT_FIELD(name, kAll, std::string{});
  EXPECT_FIELD(name, kRoot, std::string{});
  EXPECT_FIELD(name, kArgs, std::string{});
  EXPECT_FIELD(name, kMinTime, std::string{});
  EXPECT_FIELD(name, kIterations, std::string{});
  EXPECT_FIELD(name, kRepetitions, std::string{});
  EXPECT_FIELD(name, kTimeType, std::string{});
  EXPECT_FIELD(name, kThreads, std::string{});
}

TEST(BenchmarkNameTest, Root) {
  const auto name = BenchmarkName("root");

  EXPECT_FIELD(name, kAll, "root");
  EXPECT_FIELD(name, kRoot, "root");
  EXPECT_FIELD(name, kArgs, std::string{});
  EXPECT_FIELD(name, kMinTime, std::string{});
  EXPECT_FIELD(name, kIterations, std::string{});
  EXPECT_FIELD(name, kRepetitions, std::string{});
  EXPECT_FIELD(name, kTimeType, std::string{});
  EXPECT_FIELD(name, kThreads, std::string{});
}

TEST(BenchmarkNameTest, AppendToRoot) {
  const auto name =
      BenchmarkName("root").append(BenchmarkName::kRoot, "subroot");

  EXPECT_FIELD(name, kAll, "root/subroot");
  EXPECT_FIELD(name, kRoot, "root/subroot");
  EXPECT_FIELD(name, kArgs, std::string{});
  EXPECT_FIELD(name, kMinTime, std::string{});
  EXPECT_FIELD(name, kIterations, std::string{});
  EXPECT_FIELD(name, kRepetitions, std::string{});
  EXPECT_FIELD(name, kTimeType, std::string{});
  EXPECT_FIELD(name, kThreads, std::string{});
}

TEST(BenchmarkNameTest, RootAndArgs) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::kRoot, "subroot")
                        .append(BenchmarkName::kArgs, "some_args:3/4/5");

  EXPECT_FIELD(name, kAll, "root/subroot/some_args:3/4/5");
  EXPECT_FIELD(name, kRoot, "root/subroot");
  EXPECT_FIELD(name, kArgs, "some_args:3/4/5");
  EXPECT_FIELD(name, kMinTime, std::string{});
  EXPECT_FIELD(name, kIterations, std::string{});
  EXPECT_FIELD(name, kRepetitions, std::string{});
  EXPECT_FIELD(name, kTimeType, std::string{});
  EXPECT_FIELD(name, kThreads, std::string{});
}

TEST(BenchmarkNameTest, MultipleArgs) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::kArgs, "some_args:3")
                        .append(BenchmarkName::kArgs, "4")
                        .append(BenchmarkName::kArgs, "5");

  EXPECT_FIELD(name, kAll, "root/some_args:3/4/5");
  EXPECT_FIELD(name, kRoot, "root");
  EXPECT_FIELD(name, kArgs, "some_args:3/4/5");
  EXPECT_FIELD(name, kMinTime, std::string{});
  EXPECT_FIELD(name, kIterations, std::string{});
  EXPECT_FIELD(name, kRepetitions, std::string{});
  EXPECT_FIELD(name, kTimeType, std::string{});
  EXPECT_FIELD(name, kThreads, std::string{});
}

TEST(BenchmarkNameTest, MinTime) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::kArgs, "some_args:3/4")
                        .append(BenchmarkName::kMinTime, "min_time:3.4s");

  EXPECT_FIELD(name, kAll, "root/some_args:3/4/min_time:3.4s");
  EXPECT_FIELD(name, kRoot, "root");
  EXPECT_FIELD(name, kArgs, "some_args:3/4");
  EXPECT_FIELD(name, kMinTime, "min_time:3.4s");
  EXPECT_FIELD(name, kIterations, std::string{});
  EXPECT_FIELD(name, kRepetitions, std::string{});
  EXPECT_FIELD(name, kTimeType, std::string{});
  EXPECT_FIELD(name, kThreads, std::string{});
}

TEST(BenchmarkNameTest, Iterations) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::kMinTime, "min_time:3.4s")
                        .append(BenchmarkName::kIterations, "iterations:42");

  EXPECT_FIELD(name, kAll, "root/min_time:3.4s/iterations:42");
  EXPECT_FIELD(name, kRoot, "root");
  EXPECT_FIELD(name, kArgs, std::string{});
  EXPECT_FIELD(name, kMinTime, "min_time:3.4s");
  EXPECT_FIELD(name, kIterations, "iterations:42");
  EXPECT_FIELD(name, kRepetitions, std::string{});
  EXPECT_FIELD(name, kTimeType, std::string{});
  EXPECT_FIELD(name, kThreads, std::string{});
}

TEST(BenchmarkNameTest, Repetitions) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::kIterations, "iterations:42")
                        .append(BenchmarkName::kRepetitions, "repetitions:24");

  EXPECT_FIELD(name, kAll, "root/iterations:42/repetitions:24");
  EXPECT_FIELD(name, kRoot, "root");
  EXPECT_FIELD(name, kArgs, std::string{});
  EXPECT_FIELD(name, kMinTime, std::string{});
  EXPECT_FIELD(name, kIterations, "iterations:42");
  EXPECT_FIELD(name, kRepetitions, "repetitions:24");
  EXPECT_FIELD(name, kTimeType, std::string{});
  EXPECT_FIELD(name, kThreads, std::string{});
}

TEST(BenchmarkNameTest, TimeType) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::kRepetitions, "repetitions:24")
                        .append(BenchmarkName::kTimeType, "hammer_time");

  EXPECT_FIELD(name, kAll, "root/repetitions:24/hammer_time");
  EXPECT_FIELD(name, kRoot, "root");
  EXPECT_FIELD(name, kArgs, std::string{});
  EXPECT_FIELD(name, kMinTime, std::string{});
  EXPECT_FIELD(name, kIterations, std::string{});
  EXPECT_FIELD(name, kRepetitions, "repetitions:24");
  EXPECT_FIELD(name, kTimeType, "hammer_time");
  EXPECT_FIELD(name, kThreads, std::string{});
}

TEST(BenchmarkNameTest, Threads) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::kTimeType, "hammer_time")
                        .append(BenchmarkName::kThreads, "threads:256");

  EXPECT_FIELD(name, kAll, "root/hammer_time/threads:256");
  EXPECT_FIELD(name, kRoot, "root");
  EXPECT_FIELD(name, kArgs, std::string{});
  EXPECT_FIELD(name, kMinTime, std::string{});
  EXPECT_FIELD(name, kIterations, std::string{});
  EXPECT_FIELD(name, kRepetitions, std::string{});
  EXPECT_FIELD(name, kTimeType, "hammer_time");
  EXPECT_FIELD(name, kThreads, "threads:256");
}

TEST(BenchmarkNameTest, TestReadMultipleFields) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::kArgs, "first:3")
                        .append(BenchmarkName::kArgs, "second:2")
                        .append(BenchmarkName::kMinTime, "min_time:2")
                        .append(BenchmarkName::kIterations, "iterations:3")
                        .append(BenchmarkName::kRepetitions, "repetitions:4")
                        .append(BenchmarkName::kTimeType, "hammer_time")
                        .append(BenchmarkName::kThreads, "threads:6");

  EXPECT_FIELD(name, kAll,
               "root/first:3/second:2/min_time:2/iterations:3/repetitions:4/"
               "hammer_time/threads:6");

  EXPECT_EQ(name.get(BenchmarkName::kAll & ~BenchmarkName::kRoot),
            "first:3/second:2/min_time:2/iterations:3/repetitions:4/"
            "hammer_time/threads:6");

  EXPECT_EQ(name.get(BenchmarkName::kAll & ~BenchmarkName::kArgs),
            "root/min_time:2/iterations:3/repetitions:4/hammer_time/threads:6");

  EXPECT_EQ(
      name.get(BenchmarkName::kAll & ~BenchmarkName::kMinTime),
      "root/first:3/second:2/iterations:3/repetitions:4/hammer_time/threads:6");

  EXPECT_EQ(
      name.get(BenchmarkName::kAll & ~BenchmarkName::kIterations),
      "root/first:3/second:2/min_time:2/repetitions:4/hammer_time/threads:6");

  EXPECT_EQ(
      name.get(BenchmarkName::kAll & ~BenchmarkName::kRepetitions),
      "root/first:3/second:2/min_time:2/iterations:3/hammer_time/threads:6");

  EXPECT_EQ(
      name.get(BenchmarkName::kAll & ~BenchmarkName::kTimeType),
      "root/first:3/second:2/min_time:2/iterations:3/repetitions:4/threads:6");

  EXPECT_EQ(name.get(BenchmarkName::kAll & ~BenchmarkName::kThreads),
            "root/first:3/second:2/min_time:2/iterations:3/repetitions:4/"
            "hammer_time");

  EXPECT_EQ(name.get(BenchmarkName::kAll &
                     ~(BenchmarkName::kThreads | BenchmarkName::kArgs)),
            "root/min_time:2/iterations:3/repetitions:4/hammer_time");

  EXPECT_EQ(name.get(BenchmarkName::kRoot | BenchmarkName::kThreads),
            "root/threads:6");
}

TEST(BenchmarkNameTest, TestEmptyRoot) {
  const auto name = BenchmarkName(std::string{})
                        .append(BenchmarkName::kArgs, "first:3")
                        .append(BenchmarkName::kArgs, "second:4")
                        .append(BenchmarkName::kThreads, "threads:22");

  EXPECT_FIELD(name, kAll, "first:3/second:4/threads:22");
  EXPECT_FIELD(name, kArgs, "first:3/second:4");
  EXPECT_FIELD(name, kThreads, "threads:22");
}

#undef EXPECT_FIELD

}  // end namespace
