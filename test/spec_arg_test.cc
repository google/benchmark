#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"

// Tests that we can override benchmark-spec value from FLAGS_benchmark_filter
// with argument to RunSpecifiedBenchmarks(...).

namespace {

class TestReporter : public benchmark::ConsoleReporter {
 public:
  virtual bool ReportContext(const Context& context) BENCHMARK_OVERRIDE {
    return ConsoleReporter::ReportContext(context);
  };

  virtual void ReportRuns(const std::vector<Run>& report) BENCHMARK_OVERRIDE {
    assert(report.size() == 1);
    matched_functions.push_back(report[0].run_name.function_name);
    ConsoleReporter::ReportRuns(report);
  };

  TestReporter() : {}

  virtual ~TestReporter() {}

  const std::vector<std::string>& GetMatchedFunctions() const {
    return matched_functions;
  }

 private:
  std::vector<std::string> matched_functions;
};

}  // end namespace

static void BM_NotChosen(benchmark::State& state) {
  assert(false && "SHOULD NOT BE CALLED");
  for (auto _ : state) {
  }
}
BENCHMARK(BM_NotChosen);

static void BM_Chosen(benchmark::State& state) {
  for (auto _ : state) {
  }
}
BENCHMARK(BM_Chosen);

int main(int argc, char** argv) {
  bool list_only = false;

  FLAGS_benchmark_filter = "BM_NotChosen";
  benchmark::Initialize(&argc, argv);

  // Check that the current flag value is reported
  // accurately.
  if (FLAGS_benchmark_filter != benchmark::GetBenchmarkFilter()) {
    std::cerr << "Unexpected values. FLAGS_benchmark_filter= "
              << FLAGS_benchmark_filter
              << " != " << benchmark::GetBenchmarkFilter() << "\n";
    return 1;
  }
  TestReporter test_reporter;
  const size_t returned_count =
      benchmark::RunSpecifiedBenchmarks(&test_reporter,
                                        /*spec=*/"BM_Chosen");

  const std::vector<std::string> matched_functions =
      test_reporter.GetMatchedFunctions();
  assert(matched_functions.size() == 1);
  return "BM_Chosen" == matched_functions.front() ? 0 : 2;
}
