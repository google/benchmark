// Setup()/Teardown() must run outside the MemoryManager Start()/Stop()
// window, as they already do outside the timed region. See #2149.

#include <vector>

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace benchmark {
namespace {

// Everything below runs on the main thread, so no synchronization is needed.
bool in_measurement_window = false;

// Recorded so tests fail rather than pass vacuously when a code path is
// never reached.
bool memory_manager_ran = false;
bool setup_ran = false;
bool teardown_ran = false;

void ResetState() {
  in_measurement_window = false;
  memory_manager_ran = false;
  setup_ran = false;
  teardown_ran = false;
}

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

void BM_skip_before_loop(State& state) {
  state.SkipWithError("skip before loop");
  for (auto _ : state) {
  }
}
BENCHMARK(BM_skip_before_loop)->Iterations(1);

// Records reported runs without printing.
class RecordingReporter : public BenchmarkReporter {
 public:
  bool ReportContext(const Context&) override { return true; }
  void ReportRuns(const std::vector<Run>& runs) override {
    recorded.insert(recorded.end(), runs.begin(), runs.end());
  }
  std::vector<Run> recorded;
};

}  // namespace

TEST(MemoryManagerOrdering, SetupTeardownRunOutsideMeasurementWindow) {
  ResetState();
  OrderingMemoryManager mm;
  RegisterMemoryManager(&mm);
  RecordingReporter reporter;
  const size_t ran = RunSpecifiedBenchmarks(&reporter, "^BM_ordering/");
  RegisterMemoryManager(nullptr);

  EXPECT_GT(ran, 0u);
  EXPECT_TRUE(memory_manager_ran) << "MemoryManager measurement pass never ran";
  EXPECT_TRUE(setup_ran) << "Setup callback never ran";
  EXPECT_TRUE(teardown_ran) << "Teardown callback never ran";
  EXPECT_FALSE(in_measurement_window);
}

TEST(MemoryManagerOrdering, SkippedBeforeLoopCurrentlyRunsMemoryManager) {
  ResetState();
  OrderingMemoryManager mm;
  RegisterMemoryManager(&mm);
  RecordingReporter reporter;
  const size_t ran = RunSpecifiedBenchmarks(&reporter, "^BM_skip_before_loop/");
  RegisterMemoryManager(nullptr);

  EXPECT_EQ(ran, 1u);
  // Current behavior: DoOneRepetition() runs RunMemoryManager() even when the
  // timed run was skipped, though CreateRunReport() discards the result.
  EXPECT_TRUE(memory_manager_ran);
  ASSERT_EQ(reporter.recorded.size(), 1u);
  // Memory runner results were discarded.
  EXPECT_EQ(reporter.recorded[0].memory_result.memory_iterations, 0);
}

}  // namespace benchmark
