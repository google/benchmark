#undef NDEBUG

#include <cassert>
#include <vector>

#include "../src/check.h"  // NOTE: check.h is for internal use only!
#include "benchmark/benchmark.h"

namespace {

class TestReporter : public benchmark::ConsoleReporter {
 public:
  bool ReportContext(const Context& context) override {
    return ConsoleReporter::ReportContext(context);
  };

  void ReportRuns(const std::vector<Run>& report) override {
    all_runs_.insert(all_runs_.end(), begin(report), end(report));
    ConsoleReporter::ReportRuns(report);
  }

  TestReporter() {}
  ~TestReporter() override {}

  mutable std::vector<Run> all_runs_;
};

struct TestCase {
  std::string name;

  typedef benchmark::BenchmarkReporter::Run Run;

  void CheckRun(Run const& run) const {
    BM_CHECK(name == run.benchmark_name())
        << "expected " << name << " got " << run.benchmark_name();
  }
};

std::vector<TestCase> ExpectedResults;

int AddCases(const std::string& base_name,
             std::initializer_list<TestCase> const& v) {
  for (auto TC : v) {
    TC.name = base_name + TC.name;
    ExpectedResults.push_back(std::move(TC));
  }
  return 0;
}

#define CONCAT(x, y) CONCAT2(x, y)
#define CONCAT2(x, y) x##y
#define ADD_CASES(...) int CONCAT(dummy, __LINE__) = AddCases(__VA_ARGS__)

}  // end namespace

// ============== test case we non base 2 args ============== //
void BM_non_base_two_args(benchmark::State& state) {
  for (auto _ : state) {
  }
}
BENCHMARK(BM_non_base_two_args)->Arg(9)->Arg(19)->Arg(24)->Arg(1023);
ADD_CASES("BM_non_base_two_args", {{"/9"}, {"/19"}, {"/24"}, {"/1023"}});

// ============== test case we base 2 args ============== //
void BM_base_two_args(benchmark::State& state) {
  for (auto _ : state) {
  }
}
BENCHMARK(BM_base_two_args)->RangeMultiplier(2)->Range(1, 2 << 7 - 1);
ADD_CASES("BM_base_two_args", {{"/1"},
                               {"/2"},
                               {"/4"},
                               {"/8"},
                               {"/16"},
                               {"/32"},
                               {"/64"},
                               {"/128"}});

// ============== test case we base 10 args ============== //
void BM_base_ten_args(benchmark::State& state) {
  for (auto _ : state) {
  }
}
BENCHMARK(BM_base_ten_args)->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000)->Arg(32000)->Arg(100000)->Arg(1000000)->Arg(1000000000);
ADD_CASES("BM_base_ten_args", {{"/1"},
                               {"/10"},
                               {"/100"},
                               {"/1000"},
                               {"/10000"},
                               {"/32000"},
                               {"/100000"},
                               {"/1000000"},
                               {"/1000000000"}});

int main(int argc, char* argv[]) {
  benchmark::Initialize(&argc, argv);

  TestReporter test_reporter;
  benchmark::RunSpecifiedBenchmarks(&test_reporter);

  typedef benchmark::BenchmarkReporter::Run Run;

  auto EB = ExpectedResults.begin();

  for (Run const& run : test_reporter.all_runs_) {
    assert(EB != ExpectedResults.end());
    EB->CheckRun(run);
    ++EB;
  }
  assert(EB == ExpectedResults.end());

  return 0;
}
