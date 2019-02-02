#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace {

using namespace benchmark;
using namespace benchmark::internal;

#define EXPECT_FIELD(name, field, expected) \
  EXPECT_EQ((name.get(BenchmarkName::field)), (expected));

TEST(BenchmarkNameTest, Empty) {
  const auto name = BenchmarkName();

  EXPECT_FIELD(name, ALL, std::string{});
  EXPECT_FIELD(name, ROOT, std::string{});
  EXPECT_FIELD(name, ARGS, std::string{});
  EXPECT_FIELD(name, MIN_TIME, std::string{});
  EXPECT_FIELD(name, ITERATIONS, std::string{});
  EXPECT_FIELD(name, REPETITIONS, std::string{});
  EXPECT_FIELD(name, TIME_TYPE, std::string{});
  EXPECT_FIELD(name, THREADS, std::string{});
}

TEST(BenchmarkNameTest, Root) {
  const auto name = BenchmarkName("root");

  EXPECT_FIELD(name, ALL, "root");
  EXPECT_FIELD(name, ROOT, "root");
  EXPECT_FIELD(name, ARGS, std::string{});
  EXPECT_FIELD(name, MIN_TIME, std::string{});
  EXPECT_FIELD(name, ITERATIONS, std::string{});
  EXPECT_FIELD(name, REPETITIONS, std::string{});
  EXPECT_FIELD(name, TIME_TYPE, std::string{});
  EXPECT_FIELD(name, THREADS, std::string{});
}

TEST(BenchmarkNameTest, AppendToRoot) {
  const auto name =
      BenchmarkName("root").append(BenchmarkName::ROOT, "subroot");

  EXPECT_FIELD(name, ALL, "root/subroot");
  EXPECT_FIELD(name, ROOT, "root/subroot");
  EXPECT_FIELD(name, ARGS, std::string{});
  EXPECT_FIELD(name, MIN_TIME, std::string{});
  EXPECT_FIELD(name, ITERATIONS, std::string{});
  EXPECT_FIELD(name, REPETITIONS, std::string{});
  EXPECT_FIELD(name, TIME_TYPE, std::string{});
  EXPECT_FIELD(name, THREADS, std::string{});
}

TEST(BenchmarkNameTest, RootAndArgs) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::ROOT, "subroot")
                        .append(BenchmarkName::ARGS, "some_args:3/4/5");

  EXPECT_FIELD(name, ALL, "root/subroot/some_args:3/4/5");
  EXPECT_FIELD(name, ROOT, "root/subroot");
  EXPECT_FIELD(name, ARGS, "some_args:3/4/5");
  EXPECT_FIELD(name, MIN_TIME, std::string{});
  EXPECT_FIELD(name, ITERATIONS, std::string{});
  EXPECT_FIELD(name, REPETITIONS, std::string{});
  EXPECT_FIELD(name, TIME_TYPE, std::string{});
  EXPECT_FIELD(name, THREADS, std::string{});
}

TEST(BenchmarkNameTest, MultipleArgs) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::ARGS, "some_args:3")
                        .append(BenchmarkName::ARGS, "4")
                        .append(BenchmarkName::ARGS, "5");

  EXPECT_FIELD(name, ALL, "root/some_args:3/4/5");
  EXPECT_FIELD(name, ROOT, "root");
  EXPECT_FIELD(name, ARGS, "some_args:3/4/5");
  EXPECT_FIELD(name, MIN_TIME, std::string{});
  EXPECT_FIELD(name, ITERATIONS, std::string{});
  EXPECT_FIELD(name, REPETITIONS, std::string{});
  EXPECT_FIELD(name, TIME_TYPE, std::string{});
  EXPECT_FIELD(name, THREADS, std::string{});
}

TEST(BenchmarkNameTest, MinTime) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::ARGS, "some_args:3/4")
                        .append(BenchmarkName::MIN_TIME, "min_time:3.4s");

  EXPECT_FIELD(name, ALL, "root/some_args:3/4/min_time:3.4s");
  EXPECT_FIELD(name, ROOT, "root");
  EXPECT_FIELD(name, ARGS, "some_args:3/4");
  EXPECT_FIELD(name, MIN_TIME, "min_time:3.4s");
  EXPECT_FIELD(name, ITERATIONS, std::string{});
  EXPECT_FIELD(name, REPETITIONS, std::string{});
  EXPECT_FIELD(name, TIME_TYPE, std::string{});
  EXPECT_FIELD(name, THREADS, std::string{});
}

TEST(BenchmarkNameTest, Iterations) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::MIN_TIME, "min_time:3.4s")
                        .append(BenchmarkName::ITERATIONS, "iterations:42");

  EXPECT_FIELD(name, ALL, "root/min_time:3.4s/iterations:42");
  EXPECT_FIELD(name, ROOT, "root");
  EXPECT_FIELD(name, ARGS, std::string{});
  EXPECT_FIELD(name, MIN_TIME, "min_time:3.4s");
  EXPECT_FIELD(name, ITERATIONS, "iterations:42");
  EXPECT_FIELD(name, REPETITIONS, std::string{});
  EXPECT_FIELD(name, TIME_TYPE, std::string{});
  EXPECT_FIELD(name, THREADS, std::string{});
}

TEST(BenchmarkNameTest, Repetitions) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::ITERATIONS, "iterations:42")
                        .append(BenchmarkName::REPETITIONS, "repetitions:24");

  EXPECT_FIELD(name, ALL, "root/iterations:42/repetitions:24");
  EXPECT_FIELD(name, ROOT, "root");
  EXPECT_FIELD(name, ARGS, std::string{});
  EXPECT_FIELD(name, MIN_TIME, std::string{});
  EXPECT_FIELD(name, ITERATIONS, "iterations:42");
  EXPECT_FIELD(name, REPETITIONS, "repetitions:24");
  EXPECT_FIELD(name, TIME_TYPE, std::string{});
  EXPECT_FIELD(name, THREADS, std::string{});
}

TEST(BenchmarkNameTest, TimeType) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::REPETITIONS, "repetitions:24")
                        .append(BenchmarkName::TIME_TYPE, "hammer_time");

  EXPECT_FIELD(name, ALL, "root/repetitions:24/hammer_time");
  EXPECT_FIELD(name, ROOT, "root");
  EXPECT_FIELD(name, ARGS, std::string{});
  EXPECT_FIELD(name, MIN_TIME, std::string{});
  EXPECT_FIELD(name, ITERATIONS, std::string{});
  EXPECT_FIELD(name, REPETITIONS, "repetitions:24");
  EXPECT_FIELD(name, TIME_TYPE, "hammer_time");
  EXPECT_FIELD(name, THREADS, std::string{});
}

TEST(BenchmarkNameTest, Threads) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::TIME_TYPE, "hammer_time")
                        .append(BenchmarkName::THREADS, "threads:256");

  EXPECT_FIELD(name, ALL, "root/hammer_time/threads:256");
  EXPECT_FIELD(name, ROOT, "root");
  EXPECT_FIELD(name, ARGS, std::string{});
  EXPECT_FIELD(name, MIN_TIME, std::string{});
  EXPECT_FIELD(name, ITERATIONS, std::string{});
  EXPECT_FIELD(name, REPETITIONS, std::string{});
  EXPECT_FIELD(name, TIME_TYPE, "hammer_time");
  EXPECT_FIELD(name, THREADS, "threads:256");
}

TEST(BenchmarkNameTest, TestReadMultipleFields) {
  const auto name = BenchmarkName("root")
                        .append(BenchmarkName::ARGS, "first:3")
                        .append(BenchmarkName::ARGS, "second:2")
                        .append(BenchmarkName::MIN_TIME, "min_time:2")
                        .append(BenchmarkName::ITERATIONS, "iterations:3")
                        .append(BenchmarkName::REPETITIONS, "repetitions:4")
                        .append(BenchmarkName::TIME_TYPE, "hammer_time")
                        .append(BenchmarkName::THREADS, "threads:6");

  EXPECT_FIELD(name, ALL,
               "root/first:3/second:2/min_time:2/iterations:3/repetitions:4/"
               "hammer_time/threads:6");

  EXPECT_EQ(name.get(BenchmarkName::ALL & ~BenchmarkName::ROOT),
            "first:3/second:2/min_time:2/iterations:3/repetitions:4/"
            "hammer_time/threads:6");

  EXPECT_EQ(name.get(BenchmarkName::ALL & ~BenchmarkName::ARGS),
            "root/min_time:2/iterations:3/repetitions:4/hammer_time/threads:6");

  EXPECT_EQ(
      name.get(BenchmarkName::ALL & ~BenchmarkName::MIN_TIME),
      "root/first:3/second:2/iterations:3/repetitions:4/hammer_time/threads:6");

  EXPECT_EQ(
      name.get(BenchmarkName::ALL & ~BenchmarkName::ITERATIONS),
      "root/first:3/second:2/min_time:2/repetitions:4/hammer_time/threads:6");

  EXPECT_EQ(
      name.get(BenchmarkName::ALL & ~BenchmarkName::REPETITIONS),
      "root/first:3/second:2/min_time:2/iterations:3/hammer_time/threads:6");

  EXPECT_EQ(
      name.get(BenchmarkName::ALL & ~BenchmarkName::TIME_TYPE),
      "root/first:3/second:2/min_time:2/iterations:3/repetitions:4/threads:6");

  EXPECT_EQ(name.get(BenchmarkName::ALL & ~BenchmarkName::THREADS),
            "root/first:3/second:2/min_time:2/iterations:3/repetitions:4/"
            "hammer_time");

  EXPECT_EQ(name.get(BenchmarkName::ALL &
                     ~(BenchmarkName::THREADS | BenchmarkName::ARGS)),
            "root/min_time:2/iterations:3/repetitions:4/hammer_time");

  EXPECT_EQ(name.get(BenchmarkName::ROOT | BenchmarkName::THREADS),
            "root/threads:6");
}

TEST(BenchmarkNameTest, TestEmptyRoot) {
  const auto name = BenchmarkName(std::string{})
                        .append(BenchmarkName::ARGS, "first:3")
                        .append(BenchmarkName::ARGS, "second:4")
                        .append(BenchmarkName::THREADS, "threads:22");

  EXPECT_FIELD(name, ALL, "first:3/second:4/threads:22");
  EXPECT_FIELD(name, ARGS, "first:3/second:4");
  EXPECT_FIELD(name, THREADS, "threads:22");
}

#undef EXPECT_FIELD

}  // end namespace
