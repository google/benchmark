// Setup()/Teardown() must run outside the MemoryManager Start()/Stop()
// window, as they already do outside the timed region. See #2149.

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
  EXPECT_FALSE(in_measurement_window);
}

}  // namespace benchmark
