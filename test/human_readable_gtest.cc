//===---------------------------------------------------------------------===//
// human_readable_test - Unit tests for human readable converters
//===---------------------------------------------------------------------===//

#include "../src/string_util.h"
#include "gtest/gtest.h"

namespace {

TEST(HumanReadableTest, base2) {
  for (int i = 2; i < 10; ++i) {
    const auto res = benchmark::Base2HumanReadableFormat(1 << i);

    const std::string expected = "2^" + std::to_string(i);
    EXPECT_STREQ(res.c_str(), expected.c_str());
  }
}

TEST(HumanReadableTest, base10_100) {
  const auto res = benchmark::Base10HumanReadableFormat(100);
  EXPECT_STREQ(res.c_str(), "100");
}

TEST(HumanReadableTest, base10_1k) {
  const auto res = benchmark::Base10HumanReadableFormat(1000);
  EXPECT_STREQ(res.c_str(), "1k");
}

TEST(HumanReadableTest, base10_10k) {
  const auto res = benchmark::Base10HumanReadableFormat(10000);
  EXPECT_STREQ(res.c_str(), "10k");
}

TEST(HumanReadableTest, base10_20k) {
  const auto res = benchmark::Base10HumanReadableFormat(20000);
  EXPECT_STREQ(res.c_str(), "20k");
}

TEST(HumanReadableTest, base10_32k) {
  const auto res = benchmark::Base10HumanReadableFormat(32000);
  EXPECT_STREQ(res.c_str(), "32k");
}

TEST(HumanReadableTest, base10_1M) {
  const auto res = benchmark::Base10HumanReadableFormat(1000000);
  EXPECT_STREQ(res.c_str(), "1M");
}

TEST(HumanReadableTest, base10_42M) {
  const auto res = benchmark::Base10HumanReadableFormat(42000000);
  EXPECT_STREQ(res.c_str(), "42M");
}

TEST(HumanReadableTest, base10_1B) {
  const auto res = benchmark::Base10HumanReadableFormat(1000000000);
  EXPECT_STREQ(res.c_str(), "1B");
}

TEST(HumanReadableTest, base10_4B) {
  const auto res = benchmark::Base10HumanReadableFormat(4000000000);
  EXPECT_STREQ(res.c_str(), "4B");
}

TEST(HumanReadableTest, base10_4_2B) {
  const auto res = benchmark::Base10HumanReadableFormat(4200000000);
  EXPECT_STREQ(res.c_str(), "4.2B");
}

TEST(HumanReadableTest, base10_40_2M) {
  const auto res = benchmark::Base10HumanReadableFormat(40200000);
  EXPECT_STREQ(res.c_str(), "40.2M");
}

TEST(HumanReadableTest, base10_4_2Qi) {
  const auto res = benchmark::Base10HumanReadableFormat(4200000000000000000);
  EXPECT_STREQ(res.c_str(), "4.2Qi");
}

}  // end namespace
