#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

static bool have_entered_main = false;
static int num_constructed = 0;

class TestStaticInitFixture : public ::benchmark::Fixture {
  // we have to use this ugly function because ASSERT_TRUE() expands to "return
  // void", which can not be used in ctors
  void TestStaticInitFixtureRetVoid() {
    ++num_constructed;
    ASSERT_TRUE(have_entered_main);
  }

 public:
  TestStaticInitFixture() { TestStaticInitFixtureRetVoid(); }
};

BENCHMARK_DEFINE_F(TestStaticInitFixture, ConstructionHappensAtRuntime)
(benchmark::State &st) {
  for (auto _ : st) {
  }
}

BENCHMARK_REGISTER_F(TestStaticInitFixture, ConstructionHappensAtRuntime)
    ->Arg(42);

BENCHMARK_F(TestStaticInitFixture, ConstructionHappensAtRuntime2)
(benchmark::State &st) {
  for (auto _ : st) {
  }
}

TEST(fixture_init, test1) {
  ASSERT_EQ(num_constructed, 0);
  have_entered_main = true;
  ::benchmark::RunSpecifiedBenchmarks();
  ASSERT_EQ(num_constructed, 2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  benchmark::Initialize(&argc, argv);
  return RUN_ALL_TESTS();
}
