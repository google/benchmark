#include <memory>
#include <queue>
#include <string>

#include "../src/check.h"
#include "../src/commandlineflags.h"
#include "../src/string_util.h"
#include "benchmark/benchmark.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace benchmark {

constexpr int repetitions = 2;

BM_DECLARE_string(benchmark_filter);
BM_DECLARE_string(benchmark_format);

class EventQueue : public std::queue<std::string> {
 public:
  void Put(const std::string& event) { push(event); }

  void Clear() {
    while (!empty()) {
      pop();
    }
  }

  bool HasNext() { return !empty(); }

  std::string Next() {
    std::string event = front();
    pop();
    return event;
  }
};

EventQueue* queue = new EventQueue();

class TestProfiler : public benchmark::Profiler {
 public:
  void Init() BENCHMARK_OVERRIDE { queue->Put("Init"); }

  void Start() BENCHMARK_OVERRIDE { queue->Put("Start"); }
  void Stop() BENCHMARK_OVERRIDE { queue->Put("Stop"); }
  void Finalize() BENCHMARK_OVERRIDE { queue->Put("Finalize"); }
};

class BenchmarkTest : public testing::Test {
 public:
  static void SetUpTestSuite() { RegisterProfiler(InitTestProfiler()); }

  static void TearDownTestSuite() { RegisterProfiler(nullptr); }
  static void SetupHook(const benchmark::State& state) { queue->Put("Setup"); }

  static void TeardownHook(const benchmark::State& state) {
    queue->Put("Teardown");
  }

  void Execute(const std::string& benchmark_filter) {
    queue->Clear();

    FLAGS_benchmark_filter = benchmark_filter;
    FLAGS_benchmark_format = "console";
    RunSpecifiedBenchmarks();
  }

 private:
  static TestProfiler* InitTestProfiler() {
    static std::unique_ptr<benchmark::TestProfiler> mm(
        new benchmark::TestProfiler());
    return mm.get();
  }
};

void BM_empty(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(state.iterations());
  }
}
BENCHMARK(BM_empty)
    ->Repetitions(repetitions)
    ->Iterations(34)
    ->Setup(BenchmarkTest::SetupHook)
    ->Teardown(BenchmarkTest::TeardownHook);

TEST_F(BenchmarkTest, Match) {
  Execute("BM_empty");
  ASSERT_EQ("Init", queue->Next());
  for (int i = 0; i < repetitions; i++) {
    ASSERT_EQ("Setup", queue->Next());
    ASSERT_EQ("Start", queue->Next());
    ASSERT_EQ("Stop", queue->Next());
    ASSERT_EQ("Teardown", queue->Next());
  }
  ASSERT_EQ("Finalize", queue->Next());
  ASSERT_TRUE(queue->empty());
}

}  // namespace benchmark
