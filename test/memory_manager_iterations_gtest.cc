#include <vector>

#include "benchmark/benchmark_api.h"
#include "benchmark/managers.h"
#include "benchmark/registration.h"
#include "benchmark/reporter.h"
#include "benchmark/state.h"
#include "gtest/gtest.h"

namespace {
int allocations = 0;

class TestMemoryManager : public benchmark::MemoryManager {
  void Start() override { allocations = 0; }
  void Stop(Result& result) override { result.num_allocs = allocations; }
};

class TestReporter : public benchmark::BenchmarkReporter {
 public:
  bool ReportContext(const Context& /*context*/) override { return true; }
  void ReportRuns(const std::vector<Run>& runs) override {
    for (const auto& run : runs) {
      const auto expected = run.run_name.args == "10" ? 20 : 16;
      EXPECT_EQ(run.memory_result.memory_iterations, expected);
      EXPECT_EQ(run.memory_result.num_allocs, expected);
      EXPECT_DOUBLE_EQ(run.allocs_per_iter, 1.0);
      ++reported;
    }
  }
  int reported = 0;
};

void BM_MemoryIterations(benchmark::State& state) {
  const int batch = static_cast<int>(state.range(0));
  while (state.KeepRunningBatch(batch)) {
    allocations += batch;
  }
}
BENCHMARK(BM_MemoryIterations)->Arg(1)->Arg(10)->Iterations(20);
}  // namespace

TEST(MemoryManagerIterations, ReportsExecutedIterations) {
  TestMemoryManager memory_manager;
  benchmark::RegisterMemoryManager(&memory_manager);
  TestReporter reporter;
  const auto count = benchmark::RunSpecifiedBenchmarks(&reporter);
  benchmark::RegisterMemoryManager(nullptr);
  EXPECT_EQ(count, 2u);
  EXPECT_EQ(reporter.reported, 2);
}
