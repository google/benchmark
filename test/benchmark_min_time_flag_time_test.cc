#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"

// Tests that we can specify the min time with
// --benchmark_min_time=<NUM> (no suffix needed) OR
// --benchmark_min_time=<NUM>s
namespace {

class TestReporter : public benchmark::ConsoleReporter {
 public:
  virtual bool ReportContext(const Context& context) BENCHMARK_OVERRIDE {
    return ConsoleReporter::ReportContext(context);
  };

  virtual void ReportRuns(const std::vector<Run>& report) BENCHMARK_OVERRIDE {
    assert(report.size() == 1);
    min_times_.push_back(report[0].run_name.min_time);
    ConsoleReporter::ReportRuns(report);
  };

  TestReporter() {}

  virtual ~TestReporter() {}

  const std::vector<std::string>& GetMinTimes() const { return min_times_; }

 private:
  std::vector<std::string> min_times_;
};

void DoTestHelper(int* argc, const char** argv, const std::string& expected) {
  benchmark::Initialize(argc, const_cast<char**>(argv));

  TestReporter test_reporter;
  const size_t returned_count =
      benchmark::RunSpecifiedBenchmarks(&test_reporter, "BM_MyBench");
  assert(returned_count == 1);

  // Check the min_time
  const std::vector<std::string>& min_times = test_reporter.GetMinTimes();
  assert(!min_times.empty() && min_times[0] == expected);
}

}  // end namespace

static void BM_MyBench(benchmark::State& state) {
  for (auto s : state) {
  }
}
BENCHMARK(BM_MyBench);

int main(int argc, char** argv) {
  // Make a fake argv and append the new --benchmark_min_time=<foo> to it.
  int fake_argc = argc + 1;
  const char** fake_argv = new const char*[fake_argc];

  for (int i = 0; i < argc; ++i) fake_argv[i] = argv[i];

  const char* no_suffix = "--benchmark_min_time=4.0";
  const char* with_suffix = "--benchmark_min_time=4.0s";
  std::string expected = "min_time:4.0s";

  fake_argv[argc] = no_suffix;
  DoTestHelper(&fake_argc, fake_argv, expected);

  fake_argv[argc] = with_suffix;
  DoTestHelper(&fake_argc, fake_argv, expected);

  return 0;
}
