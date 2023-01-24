#include "../src/benchmark_runner.h"
#include "gtest/gtest.h"

namespace {

TEST(ParseMinTimeTest, InvalidInput) {
  // Tests only runnable in debug mode (when BM_CHECK is enabled).
#ifndef NDEBUG
  ASSERT_DEATH({ benchmark::internal::ParseBenchMinTime("abc"); },
               "Malformed seconds value passed to --benchmark_min_time: `abc`");

  ASSERT_DEATH(
      { benchmark::internal::ParseBenchMinTime("123ms"); },
      "Malformed seconds value passed to --benchmark_min_time: `123ms`");

  ASSERT_DEATH({ benchmark::internal::ParseBenchMinTime("1z"); },
               "Malformed seconds value passed to --benchmark_min_time: `1z`");

  ASSERT_DEATH({ benchmark::internal::ParseBenchMinTime("1hs"); },
               "Malformed seconds value passed to --benchmark_min_time: `1hs`");
#endif
}
}  // namespace
