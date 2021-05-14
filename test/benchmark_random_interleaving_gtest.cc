#include <queue>
#include <string>
#include <vector>

#include "../src/benchmark_adjust_repetitions.h"
#include "../src/string_util.h"
#include "benchmark/benchmark.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

DECLARE_bool(benchmark_enable_random_interleaving);
DECLARE_string(benchmark_filter);
DECLARE_double(benchmark_random_interleaving_max_overhead);

namespace do_not_read_flag_directly {
DECLARE_int32(benchmark_repetitions);
}  // namespace do_not_read_flag_directly

namespace benchmark {
namespace internal {
namespace {

class EventQueue : public std::queue<std::string> {
 public:
  void Put(const std::string& event) {
    push(event);
  }

  void Clear() {
    while (!empty()) {
      pop();
    }
  }

  std::string Get() {
    std::string event = front();
    pop();
    return event;
  }
};

static EventQueue* queue = new EventQueue;

class NullReporter : public BenchmarkReporter {
 public:
  bool ReportContext(const Context& /*context*/) override {
    return true;
  }
  void ReportRuns(const std::vector<Run>& /* report */) override {}
};

class BenchmarkTest : public testing::Test {
 public:
  static void SetupHook(int /* num_threads */) { queue->push("Setup"); }

  static void TeardownHook(int /* num_threads */) { queue->push("Teardown"); }

  void Execute(const std::string& pattern) {
    queue->Clear();

    BenchmarkReporter* reporter = new NullReporter;
    FLAGS_benchmark_filter = pattern;
    RunSpecifiedBenchmarks(reporter);
    delete reporter;

    queue->Put("DONE");  // End marker
  }
};

static void BM_Match1(benchmark::State& state) {
  const int64_t arg = state.range(0);

  for (auto _ : state) {}
  queue->Put(StrFormat("BM_Match1/%d", static_cast<int>(arg)));
}
BENCHMARK(BM_Match1)
    ->Iterations(100)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Range(10, 80)
    ->Args({90})
    ->Args({100});

static void BM_MatchOverhead(benchmark::State& state) {
  const int64_t arg = state.range(0);

  for (auto _ : state) {}
  queue->Put(StrFormat("BM_MatchOverhead/%d", static_cast<int>(arg)));
}
BENCHMARK(BM_MatchOverhead)
    ->Iterations(100)
    ->Arg(64)
    ->Arg(80);

TEST_F(BenchmarkTest, Match1) {
  Execute("BM_Match1");
  ASSERT_EQ("BM_Match1/1", queue->Get());
  ASSERT_EQ("BM_Match1/2", queue->Get());
  ASSERT_EQ("BM_Match1/3", queue->Get());
  ASSERT_EQ("BM_Match1/10", queue->Get());
  ASSERT_EQ("BM_Match1/64", queue->Get());
  ASSERT_EQ("BM_Match1/80", queue->Get());
  ASSERT_EQ("BM_Match1/90", queue->Get());
  ASSERT_EQ("BM_Match1/100", queue->Get());
  ASSERT_EQ("DONE", queue->Get());
}

TEST_F(BenchmarkTest, Match1WithRepetition) {
  do_not_read_flag_directly::FLAGS_benchmark_repetitions = 2;

  Execute("BM_Match1/(64|80)");
  ASSERT_EQ("BM_Match1/64", queue->Get());
  ASSERT_EQ("BM_Match1/64", queue->Get());
  ASSERT_EQ("BM_Match1/80", queue->Get());
  ASSERT_EQ("BM_Match1/80", queue->Get());
  ASSERT_EQ("DONE", queue->Get());
}

TEST_F(BenchmarkTest, Match1WithRandomInterleaving) {
  FLAGS_benchmark_enable_random_interleaving = true;
  do_not_read_flag_directly::FLAGS_benchmark_repetitions = 100;
  FLAGS_benchmark_random_interleaving_max_overhead =
      std::numeric_limits<double>::infinity();

  std::vector<std::string> expected({"BM_Match1/64", "BM_Match1/80"});
  std::map<std::string, int> interleaving_count;
  Execute("BM_Match1/(64|80)");
  for (int i = 0; i < 100; ++i) {
    std::vector<std::string> interleaving;
    interleaving.push_back(queue->Get());
    interleaving.push_back(queue->Get());
    EXPECT_THAT(interleaving, testing::UnorderedElementsAreArray(expected));
    interleaving_count[StrFormat("%s,%s", interleaving[0].c_str(),
                                 interleaving[1].c_str())]++;
  }
  EXPECT_GE(interleaving_count.size(), 2) << "Interleaving was not randomized.";
  ASSERT_EQ("DONE", queue->Get());
}

TEST_F(BenchmarkTest, Match1WithRandomInterleavingAndZeroOverhead) {
  FLAGS_benchmark_enable_random_interleaving = true;
  do_not_read_flag_directly::FLAGS_benchmark_repetitions = 100;
  FLAGS_benchmark_random_interleaving_max_overhead = 0;

  // ComputeRandomInterleavingRepetitions() will kick in and rerun each
  // benchmark once with increased iterations. Then number of repetitions will
  // be reduced to < 100. The first 4 executions should be
  // 2 x BM_MatchOverhead/64 and 2 x BM_MatchOverhead/80.
  std::vector<std::string> expected(
      {"BM_MatchOverhead/64", "BM_MatchOverhead/80", "BM_MatchOverhead/64",
       "BM_MatchOverhead/80"});
  std::map<std::string, int> interleaving_count;
  Execute("BM_MatchOverhead/(64|80)");
  std::vector<std::string> interleaving;
  interleaving.push_back(queue->Get());
  interleaving.push_back(queue->Get());
  interleaving.push_back(queue->Get());
  interleaving.push_back(queue->Get());
  EXPECT_THAT(interleaving, testing::UnorderedElementsAreArray(expected));
  ASSERT_LT(queue->size(), 100) << "# Repetitions was not reduced to < 100.";
}

InternalRandomInterleavingRepetitionsInput CreateInput(
    double total, double time, double real_time, double min_time,
    double overhead, int repetitions) {
  InternalRandomInterleavingRepetitionsInput input;
  input.total_execution_time_per_repetition = total;
  input.time_used_per_repetition = time;
  input.real_time_used_per_repetition = real_time;
  input.min_time_per_repetition = min_time;
  input.max_overhead = overhead;
  input.max_repetitions = repetitions;
  return input;
}

TEST(Benchmark, ComputeRandomInterleavingRepetitions) {
  // On wall clock time.
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.05, 0.05, 0.05, 0.05, 0.0, 10)),
            10);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.05, 0.05, 0.05, 0.05, 0.4, 10)),
            10);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.06, 0.05, 0.05, 0.05, 0.0, 10)),
            8);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.06, 0.05, 0.05, 0.05, 0.4, 10)),
            10);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.08, 0.05, 0.05, 0.05, 0.0, 10)),
            6);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.08, 0.05, 0.05, 0.05, 0.4, 10)),
            9);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.26, 0.25, 0.25, 0.05, 0.0, 10)),
            2);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.25, 0.25, 0.25, 0.05, 0.4, 10)),
            3);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.26, 0.25, 0.25, 0.05, 0.0, 10)),
            2);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.26, 0.25, 0.25, 0.05, 0.4, 10)),
            3);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.38, 0.25, 0.25, 0.05, 0.0, 10)),
            2);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.38, 0.25, 0.25, 0.05, 0.4, 10)),
            3);

  // On CPU time.
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.1, 0.05, 0.1, 0.05, 0.0, 10)),
            10);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.1, 0.05, 0.1, 0.05, 0.4, 10)),
            10);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.11, 0.05, 0.1, 0.05, 0.0, 10)),
            9);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.11, 0.05, 0.1, 0.05, 0.4, 10)),
            10);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.15, 0.05, 0.1, 0.05, 0.0, 10)),
            7);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.15, 0.05, 0.1, 0.05, 0.4, 10)),
            9);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.5, 0.25, 0.5, 0.05, 0.0, 10)),
            2);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.5, 0.25, 0.5, 0.05, 0.4, 10)),
            3);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.51, 0.25, 0.5, 0.05, 0.0, 10)),
            2);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.51, 0.25, 0.5, 0.05, 0.4, 10)),
            3);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.8, 0.25, 0.5, 0.05, 0.0, 10)),
            2);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.8, 0.25, 0.5, 0.05, 0.4, 10)),
            2);

  // Corner cases.
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.0, 0.25, 0.5, 0.05, 0.4, 10)),
            3);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.8, 0.0, 0.5, 0.05, 0.4, 10)),
            9);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.8, 0.25, 0.0, 0.05, 0.4, 10)),
            1);
  EXPECT_EQ(ComputeRandomInterleavingRepetitions(
                CreateInput(0.8, 0.25, 0.5, 0.0, 0.4, 10)),
            1);
}

}  // namespace
}  // namespace internal
}  // namespace benchmark
