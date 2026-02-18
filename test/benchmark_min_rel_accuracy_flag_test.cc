#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"

// Tests that if a benchmark measures time manually, we can specify the required
// relative accuracy with --benchmark_min_rel_accuracy=<min_rel_accuracy>.
namespace {

class TestReporter : public benchmark::ConsoleReporter {
 public:
  virtual bool ReportContext(const Context& context) override {
    return ConsoleReporter::ReportContext(context);
  };

  virtual void ReportRuns(const std::vector<Run>& report) override {
    assert(report.size() == 1);
    iters_.push_back(report[0].iterations);
    real_accumulated_times_.push_back(report[0].real_accumulated_time);
    manual_accumulated_time_pow2s_.push_back(
        report[0].manual_accumulated_time_pow2);
    ConsoleReporter::ReportRuns(report);
  };

  TestReporter() {}

  virtual ~TestReporter() {}

  const std::vector<benchmark::IterationCount>& GetIters() const {
    return iters_;
  }

  const std::vector<double>& GetRealAccumulatedTimes() const {
    return real_accumulated_times_;
  }

  const std::vector<double>& GetManualAccumulatedTimePow2s() const {
    return manual_accumulated_time_pow2s_;
  }

 private:
  std::vector<benchmark::IterationCount> iters_;
  std::vector<double> real_accumulated_times_;
  std::vector<double> manual_accumulated_time_pow2s_;
};

}  // end namespace

static void BM_MyBench(benchmark::State& state) {
  static std::mt19937 rd{std::random_device{}()};
  static std::uniform_real_distribution<double> mrand(0, 1);

  for (auto s : state) {
    state.SetIterationTime(mrand(rd));
  }
}
BENCHMARK(BM_MyBench)->UseManualTime();

int main(int argc, char** argv) {
  // Make a fake argv and append the new
  // --benchmark_min_rel_accuracy=<min_rel_accuracy> to it.
  int fake_argc = argc + 2;
  const char** fake_argv = new const char*[static_cast<size_t>(fake_argc)];
  for (int i = 0; i < argc; ++i) fake_argv[i] = argv[i];
  fake_argv[argc] = "--benchmark_min_time=10s";
  fake_argv[argc + 1] = "--benchmark_min_rel_accuracy=0.01";

  benchmark::Initialize(&fake_argc, const_cast<char**>(fake_argv));

  TestReporter test_reporter;
  const size_t returned_count =
      benchmark::RunSpecifiedBenchmarks(&test_reporter, "BM_MyBench");
  assert(returned_count == 1);

  // Check the executed iters.
  const auto iters = static_cast<double>(test_reporter.GetIters()[0]);
  const double real_accumulated_time =
      test_reporter.GetRealAccumulatedTimes()[0];
  const double manual_accumulated_time_pow2 =
      test_reporter.GetManualAccumulatedTimePow2s()[0];

  const double rel_accuracy =
      std::sqrt(manual_accumulated_time_pow2 / iters -
                std::pow(real_accumulated_time / iters, 2.)) /
      (real_accumulated_time / iters) / sqrt(iters);
  assert(rel_accuracy <= 0.01);

  delete[] fake_argv;
  return 0;
}
