#include <algorithm>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

#include "benchmark/benchmark_api.h"
#include "benchmark/registration.h"
#include "benchmark/reporter.h"
#include "benchmark/state.h"

namespace {

class TestReporter : public benchmark::ConsoleReporter {
 public:
  void ReportRuns(const std::vector<Run>& report) override {
    ++count_;
    max_family_index_ = std::max(max_family_index_, report[0].family_index);
    ConsoleReporter::ReportRuns(report);
  }

  int GetCount() const { return count_; }
  int64_t GetMaxFamilyIndex() const { return max_family_index_; }

 private:
  int count_ = 0;
  int64_t max_family_index_ = 0;
};

void BM_Empty(benchmark::State& state) {
  for (auto _ : state) {
  }
}

BENCHMARK(BM_Empty)->Name("BM_Untagged");
BENCHMARK(BM_Empty)->Name("BM_Std")->Tag("std");
BENCHMARK(BM_Empty)->Name("BM_StdSlow")->Tag("std")->Tag("slow");
BENCHMARK(BM_Empty)->Name("BM_Lib1")->Tag("lib1");
BENCHMARK(BM_Empty)->Name("BM_Lib1Slow")->Tag("lib1")->Tag("slow");
}  // end namespace

// Usage: tag_filter_test [--benchmark_*...] [<expected benchmark count>]
int main(int argc, char** argv) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);

  bool list_only = false;
  for (int i = 0; i < argc; ++i) {
    list_only |= std::string(argv[i]).find("--benchmark_list_tests") !=
                 std::string::npos;
  }

  benchmark::Initialize(&argc, argv);

  TestReporter test_reporter;
  const int64_t returned_count =
      static_cast<int64_t>(benchmark::RunSpecifiedBenchmarks(&test_reporter));

  // Bazel runs every *_test.cc without arguments; just check it runs.
  if (argc != 2) {
    return 0;
  }

  std::stringstream ss(argv[1]);
  int64_t expected = 0;
  ss >> expected;

  if (returned_count != expected) {
    std::cerr << "ERROR: Expected " << expected
              << " benchmarks to match but returned_count = " << returned_count
              << '\n';
    return -1;
  }

  const int64_t expected_reports = list_only ? 0 : expected;
  if (test_reporter.GetCount() != expected_reports) {
    std::cerr << "ERROR: Expected " << expected_reports
              << " benchmarks to run but reported_count = "
              << test_reporter.GetCount() << '\n';
    return -1;
  }

  // Families skipped by tags must not leave gaps in family_index.
  const int64_t num_families =
      expected_reports == 0 ? 0 : 1 + test_reporter.GetMaxFamilyIndex();
  if (num_families != expected_reports) {
    std::cerr << "ERROR: Expected " << expected_reports
              << " families to run but num_families = " << num_families << '\n';
    return -1;
  }

  return 0;
}
