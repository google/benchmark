
#undef NDEBUG
#include "benchmark/benchmark.h"
#include "../src/check.h" // NOTE: check.h is for internal use only!
#include <cassert>
#include <vector>

namespace {

class TestReporter : public benchmark::ConsoleReporter {
 public:
  virtual bool ReportContext(const Context& context) {
    return ConsoleReporter::ReportContext(context);
  };

  virtual void ReportRuns(const std::vector<Run>& report) {
    all_runs_.insert(all_runs_.end(), begin(report), end(report));
    ConsoleReporter::ReportRuns(report);
  }

  TestReporter()  {}
  virtual ~TestReporter() {}

  mutable std::vector<Run> all_runs_;
};

struct TestCase {
  std::string name;
  const char* label;
  TestCase(const char* xname) : name(xname) {}
  TestCase(const char* xname, const char* xlabel)
    : name(xname), label(xlabel) {}

  typedef benchmark::BenchmarkReporter::Run Run;

  void CheckRun(Run const& run) const {
    CHECK(name == run.benchmark_name) << "expected " << name << " got " << run.benchmark_name;
    if (label) {
      CHECK(run.report_label == label) << "expected " << label << " got " << run.report_label;
    } else {
      CHECK(run.report_label == "");
    }
  }
};

std::vector<TestCase> ExpectedResults;

int AddCases(std::initializer_list<TestCase> const& v) {
  for (auto N : v) {
    ExpectedResults.push_back(N);
  }
  return 0;
}

#define CONCAT(x, y) CONCAT2(x, y)
#define CONCAT2(x, y) x##y
#define ADD_CASES(...) \
int CONCAT(dummy, __LINE__) = AddCases({__VA_ARGS__})

}  // end namespace

typedef benchmark::internal::Benchmark* ReturnVal;

void BM_function(benchmark::State& state) {
  while (state.KeepRunning()) {}
}
BENCHMARK(BM_function);
ReturnVal dummy = benchmark::RegisterBenchmark(
    "BM_function_manual_registration",
     BM_function);
ADD_CASES({"BM_function"}, {"BM_function_manual_registration"});


void BM_extra_args(benchmark::State& st, const char* label) {
  while (st.KeepRunning()) {}
  st.SetLabel(label);
}
int RegisterFromFunction() {
  std::pair<const char*, const char*> cases[] = {
      {"test1", "One"},
      {"test2", "Two"},
      {"test3", "Three"}
  };
  for (auto& c : cases)
    benchmark::RegisterBenchmark(c.first, &BM_extra_args, c.second);
  return 0;
}
int dummy2 = RegisterFromFunction();
ADD_CASES(
  {"test1", "One"},
  {"test2", "Two"},
  {"test3", "Three"}
);

struct CustomFixture {
  void operator()(benchmark::State& st) {
    while (st.KeepRunning()) {}
  }
};

void TestRegistrationAtRuntime() {
  {
    CustomFixture fx;
    benchmark::RegisterBenchmark("custom_fixture", fx);
    AddCases({"custom_fixture"});
  }
  {
    int x = 42;
    auto capturing_lam = [=](benchmark::State& st) {
      while (st.KeepRunning()) {}
      st.SetLabel(std::to_string(x));
    };
    benchmark::RegisterBenchmark("lambda_benchmark", capturing_lam);
    AddCases({"lambda_benchmark", "42"});
  }
}

int main(int argc, char* argv[]) {
  TestRegistrationAtRuntime();

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
