#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace benchmark {
namespace {

// Everything below runs on the main thread, so no synchronization is needed.
bool in_measurement_window = false;

// Recorded so the test fails rather than passes vacuously when a code path is
// never reached.
bool memory_manager_ran = false;
bool setup_ran = false;
bool teardown_ran = false;

class OrderingMemoryManager : public MemoryManager {
 public:
  void Start() override {
    in_measurement_window = true;
    memory_manager_ran = true;
  }
  void Stop(Result& result) override {
    in_measurement_window = false;
    result.num_allocs = 0;
    result.max_bytes_used = 0;
  }
};

void DoSetup(const State&) {
  EXPECT_FALSE(in_measurement_window)
      << "Setup ran inside the MemoryManager Start/Stop window";
  setup_ran = true;
}
void DoTeardown(const State&) {
  EXPECT_FALSE(in_measurement_window)
      << "Teardown ran inside the MemoryManager Start/Stop window";
  teardown_ran = true;
}

void BM_ordering(State& state) {
  for (auto _ : state) {
  }
}
BENCHMARK(BM_ordering)->Iterations(1)->Setup(DoSetup)->Teardown(DoTeardown);

// Regression test for #1849: multithreaded benchmarks with memory manager
// run single-threaded during the memory manager pass, and state.threads() must
// report 1 (not the benchmark's configured thread count) to prevent hangs on
// barrier synchronization.
bool memory_pass_threads_is_one = false;
bool benchmark_pass_threads_is_four = false;

class MultithreadedOrderingFixture : public Fixture {
 public:
  void SetUp(const State& state) override { Sync(state.threads()); }

  void TearDown(const State& state) override { Sync(state.threads()); }

 private:
  void Sync(int thread_count) {
    std::unique_lock<std::mutex> lock(mutex_);
    int gen = generation_;
    if (++arrived_ == thread_count) {
      arrived_ = 0;
      ++generation_;
      cv_.notify_all();
    } else {
      ASSERT_TRUE(cv_.wait_for(lock, std::chrono::seconds(5), [&] {
        return generation_ != gen;
      })) << "Timed out waiting for all threads; state.threads() was wrong";
    }
  }

  std::mutex mutex_;
  std::condition_variable cv_;
  int arrived_ = 0;
  int generation_ = 0;
};

BENCHMARK_DEFINE_F(MultithreadedOrderingFixture, BM_MultithreadedSync)
(State& state) {
  if (in_measurement_window) {
    if (state.threads() == 1) {
      memory_pass_threads_is_one = true;
    }
  } else {
    if (state.threads() == 4) {
      benchmark_pass_threads_is_four = true;
    }
  }
  for (auto _ : state) {
  }
}
BENCHMARK_REGISTER_F(MultithreadedOrderingFixture, BM_MultithreadedSync)
    ->Iterations(1)
    ->Threads(4);

// Swallows reporter output.
class NullReporter : public BenchmarkReporter {
 public:
  bool ReportContext(const Context&) override { return true; }
  void ReportRuns(const std::vector<Run>&) override {}
};

}  // namespace

TEST(MemoryManagerOrdering, SetupTeardownRunOutsideMeasurementWindow) {
  OrderingMemoryManager mm;
  RegisterMemoryManager(&mm);
  NullReporter reporter;
  const size_t ran = RunSpecifiedBenchmarks(&reporter);
  RegisterMemoryManager(nullptr);

  EXPECT_GT(ran, 0u);
  EXPECT_TRUE(memory_manager_ran) << "MemoryManager measurement pass never ran";
  EXPECT_TRUE(setup_ran) << "Setup callback never ran";
  EXPECT_TRUE(teardown_ran) << "Teardown callback never ran";
  EXPECT_TRUE(memory_pass_threads_is_one)
      << "state.threads() was not 1 during MemoryManager measurement pass";
  EXPECT_TRUE(benchmark_pass_threads_is_four)
      << "state.threads() was not 4 during normal iterations";
  EXPECT_FALSE(in_measurement_window);
}

}  // namespace benchmark
