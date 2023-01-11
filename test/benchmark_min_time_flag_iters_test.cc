#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"

// Tests that we can specify the number of iterations with
// --benchmark_min_time=<NUM>x.
namespace {

class TestReporter : public benchmark::ConsoleReporter {
 public:
  virtual bool ReportContext(const Context& context) BENCHMARK_OVERRIDE {
    return ConsoleReporter::ReportContext(context);
  };

  virtual void ReportRuns(const std::vector<Run>& report) BENCHMARK_OVERRIDE {
    assert(report.size() == 1);
    iter_nums_.push_back(report[0].iterations);
    ConsoleReporter::ReportRuns(report);
  };

  TestReporter() {}

  virtual ~TestReporter() {}

  const std::vector<int>& GetIters() const { return iter_nums_; }

 private:
  std::vector<int> iter_nums_;
};

}  // end namespace

static void BM_MyBench(benchmark::State& state) {
  for (auto s : state) {
  }
}
BENCHMARK(BM_MyBench);

int main(int argc, char** argv) {
  // Verify that argv specify --benchmark_min_time=4x
  bool found = false;
  for (int i = 0; i < argc; ++i) {
    if (strcmp("--benchmark_min_time=4x", argv[i]) == 0) {
      found = true;
      break;
    }
  }
  assert(found);
  benchmark::Initialize(&argc, argv);

  TestReporter test_reporter;
  const size_t returned_count =
      benchmark::RunSpecifiedBenchmarks(&test_reporter, "BM_MyBench");
  assert(returned_count == 1);

  // Check the executed iters.
  const std::vector<int> iters = test_reporter.GetIters();
  assert(!iters.empty() && iters[0] == 4);
  return 0;
}
