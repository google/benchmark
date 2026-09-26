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

const std::vector<BenchmarkInstance>& ListEscapedBenchmarks() {
  static const std::vector<BenchmarkInstance>* const benchmarks = [] {
    RegisterBenchmark("=FormulaInjection", BM_ReporterListDummy);
    RegisterBenchmark("+PlusPrefix", BM_ReporterListDummy);
    RegisterBenchmark("-MinusPrefix", BM_ReporterListDummy);
    RegisterBenchmark("@AtPrefix", BM_ReporterListDummy);
    RegisterBenchmark("BM_ANSI_\x1b[31mRed\x1b[0m", BM_ReporterListDummy);
    auto* result = new std::vector<BenchmarkInstance>();
    std::ostringstream err_stream;
    FindBenchmarksInternal(
        "(=FormulaInjection|\\+PlusPrefix|-MinusPrefix|@AtPrefix|BM_ANSI_).*",
        result, &err_stream);
    return result;
  }();
  return *benchmarks;
}

TEST(ReporterListTest, CSVEscapesFormulaPrefixes) {
  BENCHMARK_DISABLE_DEPRECATED_WARNING
  CSVReporter reporter;
  std::ostringstream out;
  reporter.SetOutputStream(&out);
  reporter.List(ListEscapedBenchmarks());
  std::string s = out.str();
  EXPECT_NE(s.find("\"'=FormulaInjection\"\n"), std::string::npos);
  EXPECT_NE(s.find("\"'+PlusPrefix\"\n"), std::string::npos);
  EXPECT_NE(s.find("\"'-MinusPrefix\"\n"), std::string::npos);
  EXPECT_NE(s.find("\"'@AtPrefix\"\n"), std::string::npos);
  BENCHMARK_RESTORE_DEPRECATED_WARNING
}

TEST(ReporterListTest, JSONEscapesC0ControlChars) {
  JSONReporter reporter;
  std::ostringstream out;
  reporter.SetOutputStream(&out);
  reporter.List(ListEscapedBenchmarks());
  std::string s = out.str();
  EXPECT_NE(s.find("\"name\": \"BM_ANSI_\\u001b[31mRed\\u001b[0m\""),
            std::string::npos);
}

}  // namespace
}  // namespace internal
}  // namespace benchmark
