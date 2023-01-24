#include "../src/benchmark_runner.h"
#include "gtest/gtest.h"

namespace {

TEST(ParseMinTimeTest, InvalidInput) {
  ASSERT_DEATH({ benchmark::internal::ParseBenchMinTime("abc"); },
               "Malformed seconds value passed to --benchmark_min_time: `abc`");

  ASSERT_DEATH(
      { benchmark::internal::ParseBenchMinTime("123ms"); },
      "Malformed seconds value passed to --benchmark_min_time: `123ms`");

  ASSERT_DEATH({ benchmark::internal::ParseBenchMinTime("1z"); },
               "Malformed seconds value passed to --benchmark_min_time: `1z`");

  ASSERT_DEATH({ benchmark::internal::ParseBenchMinTime("1hs"); },
               "Malformed seconds value passed to --benchmark_min_time: `1hs`");
}
}  // namespace
