#include <sstream>
#include <string>
#include <vector>

#include "../src/benchmark_api_internal.h"
#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace benchmark {
namespace internal {
namespace {

void BM_ReporterListDummy(::benchmark::State& state) {
  for (auto _ : state) {
  }
}

// Registers two benchmarks (once) and returns the matching instances, the
// same way RunSpecifiedBenchmarks() selects them for --benchmark_list_tests.
const std::vector<BenchmarkInstance>& ListTestBenchmarks() {
  static const std::vector<BenchmarkInstance>* const benchmarks = [] {
    RegisterBenchmark("BM_ReporterListFirst", BM_ReporterListDummy);
    RegisterBenchmark("BM_ReporterListSecond", BM_ReporterListDummy);
    auto* result = new std::vector<BenchmarkInstance>();
    std::ostringstream err_stream;
    FindBenchmarksInternal("BM_ReporterList.*", result, &err_stream);
    return result;
  }();
  return *benchmarks;
}

template <typename Reporter>
std::string ListedOutput() {
  Reporter reporter;
  std::ostringstream out;
  reporter.SetOutputStream(&out);
  reporter.List(ListTestBenchmarks());
  return out.str();
}

TEST(ReporterListTest, FindsRegisteredBenchmarks) {
  ASSERT_EQ(ListTestBenchmarks().size(), 2u);
}

TEST(ReporterListTest, DefaultListsOneNamePerLine) {
  EXPECT_EQ(ListedOutput<ConsoleReporter>(),
            "BM_ReporterListFirst\nBM_ReporterListSecond\n");
}

TEST(ReporterListTest, JSONListsNamesInBenchmarksArray) {
  EXPECT_EQ(ListedOutput<JSONReporter>(),
            "{\n"
            "  \"benchmarks\": [\n"
            "    {\n"
            "      \"name\": \"BM_ReporterListFirst\"\n"
            "    },\n"
            "    {\n"
            "      \"name\": \"BM_ReporterListSecond\"\n"
            "    }\n"
            "  ]\n"
            "}\n");
}

TEST(ReporterListTest, JSONListsNoBenchmarks) {
  JSONReporter reporter;
  std::ostringstream out;
  reporter.SetOutputStream(&out);
  reporter.List({});
  EXPECT_EQ(out.str(),
            "{\n"
            "  \"benchmarks\": [\n"
            "  ]\n"
            "}\n");
}

TEST(ReporterListTest, CSVListsNameColumn) {
  BENCHMARK_DISABLE_DEPRECATED_WARNING
  EXPECT_EQ(ListedOutput<CSVReporter>(),
            "name\n"
            "\"BM_ReporterListFirst\"\n"
            "\"BM_ReporterListSecond\"\n");
  BENCHMARK_RESTORE_DEPRECATED_WARNING
}

}  // namespace
}  // namespace internal
}  // namespace benchmark
