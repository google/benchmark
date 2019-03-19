#include <vector>

#include "../src/benchmark_register.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

TEST(AddRangeTest, Simple) {
  std::vector<int> dst;
  AddRange(&dst, 1, 2, 2);
  EXPECT_THAT(dst, testing::ElementsAre(1, 2));
}

TEST(AddRangeTest, Simple64) {
  std::vector<int64_t> dst;
  AddRange(&dst, static_cast<int64_t>(1), static_cast<int64_t>(2), 2);
  EXPECT_THAT(dst, testing::ElementsAre(1, 2));
}

TEST(AddRangeTest, Advanced) {
  std::vector<int> dst;
  AddRange(&dst, 5, 15, 2);
  EXPECT_THAT(dst, testing::ElementsAre(5, 8, 15));
}

TEST(AddRangeTest, Advanced64) {
  std::vector<int64_t> dst;
  AddRange(&dst, static_cast<int64_t>(5), static_cast<int64_t>(15), 2);
  EXPECT_THAT(dst, testing::ElementsAre(5, 8, 15));
}

TEST(AddRangeTest, FullRange8) {
  std::vector<int8_t> dst;
  AddRange(&dst, int8_t{1}, std::numeric_limits<int8_t>::max(), 8);
  EXPECT_THAT(dst, testing::ElementsAre(1, 8, 64, 127));
}

TEST(AddRangeTest, FullRange64) {
  std::vector<int64_t> dst;
  AddRange(&dst, int64_t{1}, std::numeric_limits<int64_t>::max(), 1024);
  EXPECT_THAT(
      dst, testing::ElementsAre(1LL, 1024LL, 1048576LL, 1073741824LL,
                                1099511627776LL, 1125899906842624LL,
                                1152921504606846976LL, 9223372036854775807LL));
}

}  // end namespace
