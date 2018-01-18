#include "benchmark/benchmark.h"

static bool have_entered_main = false;
static int num_constructed = 0;

class StaticInitFixtureTest : public ::benchmark::Fixture {
 public:
  StaticInitFixtureTest() {
    ++num_constructed;
    assert(have_entered_main);
  }
};

BENCHMARK_DEFINE_F(StaticInitFixtureTest, ConstructionHappensAtRuntime)
(benchmark::State &st) {
  for (auto _ : st) {
  }
}

BENCHMARK_REGISTER_F(StaticInitFixtureTest, ConstructionHappensAtRuntime)
    ->Arg(42);

BENCHMARK_F(StaticInitFixtureTest, ConstructionHappensAtRuntime2)
(benchmark::State &st) {
  for (auto _ : st) {
  }
}

int main(int argc, char **argv) {
  benchmark::Initialize(&argc, argv);
  assert(num_constructed == 0);
  have_entered_main = true;
  ::benchmark::RunSpecifiedBenchmarks();
  assert(num_constructed == 2);
}
