// Setup()/Teardown(), pre-loop setup, and post-loop teardown must run outside
// the active MemoryManager Start()/Stop() window. See #2149.

#include <cstddef>
#include <string>
#include <vector>

#include "benchmark/benchmark_api.h"
#include "benchmark/managers.h"
#include "benchmark/reporter.h"
#include "benchmark/state.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace benchmark {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {

class OrderingMemoryManager : public MemoryManager {
 public:
  void Start() override {
    in_measurement_window = true;
    ran = true;
    events.push_back("Start");
  }

  void Stop(Result& result) override {
    EXPECT_TRUE(in_measurement_window);
    in_measurement_window = false;
    events.push_back("Stop");
    result.num_allocs = 0;
    result.max_bytes_used = 0;
  }

  bool in_measurement_window = false;
  bool ran = false;
  std::vector<std::string> events;
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

TEST_F(MemoryManagerOrdering, SetupTeardownAndPrePostLoopRunOutsideWindow) {
  OrderingMemoryManager mm;
  bool setup_ran = false;
  bool teardown_ran = false;

  RegisterBenchmark("BM_ordering",
                    [&](State& state) {
                      EXPECT_FALSE(mm.in_measurement_window)
                          << "Pre-loop setup ran inside the MemoryManager "
                             "Start/Stop window";
                      mm.events.push_back("PreLoop");
                      for (auto _ : state) {
                        mm.events.push_back("Body");
                        if (mm.ran) {
                          EXPECT_TRUE(mm.in_measurement_window);
                        }
                      }
                      mm.events.push_back("PostLoop");
                      EXPECT_FALSE(mm.in_measurement_window)
                          << "Post-loop teardown ran inside the MemoryManager "
                             "Start/Stop window";
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
  // First pass is the timed run; second pass is the memory manager run.
  EXPECT_THAT(mm.events, ElementsAre("PreLoop", "Body", "PostLoop", "PreLoop",
                                     "Start", "Body", "Stop", "PostLoop"));
}

TEST_F(MemoryManagerOrdering, KeepRunningLoopStartsAndStopsInsideLoop) {
  OrderingMemoryManager mm;

  RegisterBenchmark("BM_keep_running", [&](State& state) {
    EXPECT_FALSE(mm.in_measurement_window);
    mm.events.push_back("PreLoop");
    while (state.KeepRunning()) {
      mm.events.push_back("Body");
    }
    mm.events.push_back("PostLoop");
    EXPECT_FALSE(mm.in_measurement_window);
  })->Iterations(2);

  RegisterMemoryManager(&mm);
  RecordingReporter reporter;
  const size_t ran = RunSpecifiedBenchmarks(&reporter);

  EXPECT_EQ(ran, 1u);
  EXPECT_FALSE(mm.in_measurement_window);
  EXPECT_THAT(mm.events,
              ElementsAre("PreLoop", "Body", "Body", "PostLoop", "PreLoop",
                          "Start", "Body", "Body", "Stop", "PostLoop"));
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
  EXPECT_THAT(mm.events, IsEmpty());
  ASSERT_EQ(reporter.recorded.size(), 1u);
  EXPECT_EQ(reporter.recorded[0].memory_result.memory_iterations, 0);
}

TEST_F(MemoryManagerOrdering, SkipInsideMemoryPassPairsStartAndStop) {
  OrderingMemoryManager mm;
  bool skip_on_second_run = false;

  RegisterBenchmark("BM_skip_in_memory_pass", [&](State& state) {
    const bool should_skip = skip_on_second_run;
    skip_on_second_run = true;
    for (auto _ : state) {
      if (should_skip) {
        state.SkipWithError("skip during memory pass");
        break;
      }
    }
  })->Iterations(2);

  RegisterMemoryManager(&mm);
  RecordingReporter reporter;
  const size_t ran = RunSpecifiedBenchmarks(&reporter);

  EXPECT_EQ(ran, 1u);
  EXPECT_FALSE(mm.in_measurement_window);
  EXPECT_THAT(mm.events, ElementsAre("Start", "Stop"));
  ASSERT_EQ(reporter.recorded.size(), 1u);
  EXPECT_EQ(reporter.recorded[0].memory_result.memory_iterations, 0);
}

}  // namespace benchmark
