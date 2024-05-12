#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include "benchmark/benchmark.h"

namespace benchmark {
namespace internal {

// Helper function to register benchmarks
void RegisterBenchmarks() {
    benchmark::RegisterBenchmark("BM_simple", [](benchmark::State& state) {
        for (auto _ : state) {}
    });
    benchmark::RegisterBenchmark("BM_complex", [](benchmark::State& state) {
        for (auto _ : state) {}
    });
}

class ReporterListTest : public ::testing::Test {
protected:
    std::stringstream ss;
    BenchmarkReporter::Context context;

    void SetUp() override {
        benchmark::ClearRegisteredBenchmarks();
        ss.str("");
        ss.clear();
    }

    void RunList(BenchmarkReporter& reporter) {
        RegisterBenchmarks(); // Register all benchmarks
        benchmark::RunSpecifiedBenchmarks(&reporter);
    }
};

// Tests the behavior of reporters when no benchmarks are registered
TEST_F(ReporterListTest, HandlesEmptyBenchmarkList) {
    benchmark::ConsoleReporter console_reporter(&ss);
    benchmark::JSONReporter json_reporter(&ss);
    benchmark::CSVReporter csv_reporter(&ss);

    // Expect no output as no benchmarks are registered
    RunList(console_reporter);
    EXPECT_TRUE(ss.str().empty());

    ss.str("");
    RunList(json_reporter);
    EXPECT_TRUE(ss.str().empty());

    ss.str("");
    RunList(csv_reporter);
    EXPECT_TRUE(ss.str().empty());
}

// Tests the output of reporters when benchmarks are listed
TEST_F(ReporterListTest, ListsBenchmarksWithDifferentReporters) {
    benchmark::ConsoleReporter console_reporter(&ss);
    benchmark::JSONReporter json_reporter(&ss);
    benchmark::CSVReporter csv_reporter(&ss);

    RegisterBenchmarks(); // Ensure some benchmarks are registered

    RunList(console_reporter);
    EXPECT_THAT(ss.str(), ::testing::HasSubstr("BM_simple"));
    EXPECT_THAT(ss.str(), ::testing::HasSubstr("BM_complex"));

    ss.str("");
    RunList(json_reporter);
    EXPECT_THAT(ss.str(), ::testing::HasSubstr("\"name\": \"BM_simple\""));
    EXPECT_THAT(ss.str(), ::testing::HasSubstr("\"name\": \"BM_complex\""));

    ss.str("");
    RunList(csv_reporter);
    EXPECT_THAT(ss.str(), ::testing::HasSubstr("BM_simple"));
    EXPECT_THAT(ss.str(), ::testing::HasSubstr("BM_complex"));
}

}  // namespace internal
}  // namespace benchmark
