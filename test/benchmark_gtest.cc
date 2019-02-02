#include <vector>

#include "../src/benchmark_register.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace benchmark { namespace internal {

std::string drop_last_field (const std::string &, char);

} }

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

TEST(DropLastField, Empty) {
  std::string name;
  EXPECT_EQ(benchmark::internal::drop_last_field(name, '/'), name);
}

TEST(DropLastField, OneField) {
  std::string name("GoogleBenchmark");
  EXPECT_EQ(benchmark::internal::drop_last_field(name, '/'),
            name);
}

TEST(DropLastField, TwoField) {
  std::string name("Google/Benchmark");
  EXPECT_EQ(benchmark::internal::drop_last_field(name, '/'),
            std::string("Google"));
}

TEST(DropLastField, ThreeField) {
  std::string name("Google/Bench/mark");
  EXPECT_EQ(benchmark::internal::drop_last_field(name, '/'),
            std::string("Google/Bench"));
}

}  // end namespace
