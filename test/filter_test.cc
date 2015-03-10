#include "benchmark/benchmark.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include <iostream>
#include <sstream>
#include <string>

namespace {

double CalculatePi(int depth) {
  double pi = 0.0;
  for (int i = 0; i < depth; ++i) {
    double numerator = static_cast<double>(((i % 2) * 2) - 1);
    double denominator = static_cast<double>((2 * i) - 1);
    pi += numerator / denominator;
  }
  return (pi - 1.0) * 4;
}

class TestReporter : public benchmark::internal::ConsoleReporter {
 public:
  virtual bool ReportContext(const Context& context) const {
    return ConsoleReporter::ReportContext(context);
  };

  virtual void ReportRuns(const std::vector<Run>& report) const {
    ++count_;
    ConsoleReporter::ReportRuns(report);
  };

  TestReporter() : count_(0) {}

  virtual ~TestReporter() {}

  size_t GetCount() const {
    return count_;
  }

 private:
  mutable size_t count_;
};

}  // end namespace

static void BM_CalculatePiRange(benchmark::State& state) {
  double pi = 0.0;
  while (state.KeepRunning())
    pi = CalculatePi(state.range_x());
  std::stringstream ss;
  ss << pi;
  state.SetLabel(ss.str());
}
BENCHMARK_RANGE(BM_CalculatePiRange, 1, 1024 * 1024);

static void BM_CalculatePi(benchmark::State& state) {
  static const int depth = 1024;
  double pi BENCHMARK_UNUSED = 0.0;
  while (state.KeepRunning()) {
    pi = CalculatePi(depth);
  }
}
BENCHMARK(BM_CalculatePi)->Threads(8);
BENCHMARK(BM_CalculatePi)->ThreadRange(1, 32);
BENCHMARK(BM_CalculatePi)->ThreadPerCpu();

int main(int argc, const char* argv[]) {
  benchmark::Initialize(&argc, argv);

  assert(CalculatePi(1) == 0.0);

  TestReporter test_reporter;
  benchmark::RunSpecifiedBenchmarks(&test_reporter);

  // Make sure we ran all of the tests
  const size_t count = test_reporter.GetCount();
  const size_t expected = (argc == 2) ? std::stoul(argv[1]) : count;
  if (count != expected) {
    std::cerr << "ERROR: Expected " << expected << " tests to be ran but only "
              << count << " completed" << std::endl;
    return -1;
  }
}

