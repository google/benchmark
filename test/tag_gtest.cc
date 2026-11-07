#include "benchmark/benchmark_api.h"
#include "benchmark/state.h"
#include "gtest/gtest.h"

namespace {

void BM_Empty(benchmark::State& state) {
  for (auto _ : state) {
  }
}

// These characters are reserved by the --benchmark_tags syntax.
TEST(TagTest, RejectsInvalidTags) {
#if GTEST_HAS_DEATH_TEST
  // Checked in every build type: a bad tag would otherwise never match.
  benchmark::Benchmark* bm = benchmark::RegisterBenchmark("BM_Empty", BM_Empty);
  EXPECT_DEATH(bm->Tag(""), "must not be empty");
  EXPECT_DEATH(bm->Tag("a,b"), "must not contain ','");
  EXPECT_DEATH(bm->Tag("-a"), "must not start with '-'");
  EXPECT_DEATH(bm->Tag(" a"), "must not start or end with whitespace");
  EXPECT_DEATH(bm->Tag("a\t"), "must not start or end with whitespace");
#else
  GTEST_SKIP() << "Test skipped because death tests are not supported";
#endif
}

}  // namespace
