// Setup()/Teardown() must run outside the MemoryManager Start()/Stop()
// window, as they already do outside the timed region. See #2149.

#include <cstddef>
#include <vector>

#include "benchmark/benchmark_api.h"
#include "benchmark/managers.h"
#include "benchmark/reporter.h"
#include "benchmark/state.h"
#include "gtest/gtest.h"

namespace benchmark {
namespace {

class OrderingMemoryManager : public MemoryManager {
 public:
  void Start() override {
    in_measurement_window = true;
    ran = true;
  }

  void Stop(Result& result) override {
    EXPECT_TRUE(in_measurement_window);
    in_measurement_window = false;
    result.num_allocs = 0;
    result.max_bytes_used = 0;
  }

  bool in_measurement_window = false;
  bool ran = false;
};

// Records reported runs without printing.
class RecordingReporter : public BenchmarkReporter {
 public:
  bool ReportContext(const Context&) override { return true; }
  void ReportRuns(const std::vector<Run>& runs) override {
    recorded.insert(recorded.end(), runs.begin(), runs.end());
  }
  std::vector<Run> recorded;
};

class MemoryManagerOrdering : public testing::Test {
 protected:
  void TearDown() override {
    RegisterMemoryManager(nullptr);
    ClearRegisteredBenchmarks();
  }
};

}  // namespace

TEST_F(MemoryManagerOrdering, SetupTeardownRunOutsideMeasurementWindow) {
  OrderingMemoryManager mm;
  bool setup_ran = false;
  bool teardown_ran = false;

  RegisterBenchmark("BM_ordering",
                    [](State& state) {
                      for (auto _ : state) {
                      }
                    })
      ->Iterations(1)
      ->Setup([&](const State&) {
        EXPECT_FALSE(mm.in_measurement_window)
            << "Setup ran inside the MemoryManager Start/Stop window";
        setup_ran = true;
      })
      ->Teardown([&](const State&) {
        EXPECT_FALSE(mm.in_measurement_window)
            << "Teardown ran inside the MemoryManager Start/Stop window";
        teardown_ran = true;
      });

  RegisterMemoryManager(&mm);
  RecordingReporter reporter;
  const size_t ran = RunSpecifiedBenchmarks(&reporter);

  EXPECT_EQ(ran, 1u);
  EXPECT_TRUE(mm.ran) << "MemoryManager measurement pass never ran";
  EXPECT_TRUE(setup_ran) << "Setup callback never ran";
  EXPECT_TRUE(teardown_ran) << "Teardown callback never ran";
  EXPECT_FALSE(mm.in_measurement_window);
}

TEST_F(MemoryManagerOrdering, SkippedBeforeLoopDoesNotStartMemoryManager) {
  OrderingMemoryManager mm;

  RegisterBenchmark("BM_skip_before_loop", [](State& state) {
    state.SkipWithError("skip before loop");
    for (auto _ : state) {
    }
  })->Iterations(1);

  RegisterMemoryManager(&mm);
  RecordingReporter reporter;
  const size_t ran = RunSpecifiedBenchmarks(&reporter);

  EXPECT_EQ(ran, 1u);
  EXPECT_FALSE(mm.ran);
  ASSERT_EQ(reporter.recorded.size(), 1u);
  EXPECT_EQ(reporter.recorded[0].memory_result.memory_iterations, 0);
}

}  // namespace benchmark
