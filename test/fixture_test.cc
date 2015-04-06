
#include "benchmark/benchmark.h"

class MyFixture : public ::benchmark::Fixture
{
};


BENCHMARK_F(MyFixture, Foo)(benchmark::State& st) {
    while (st.KeepRunning()) {
    }
}

BENCHMARK_DEFINE_F(MyFixture, Bar)(benchmark::State& st) {
  while (st.KeepRunning()) {
  }
  st.SetItemsProcessed(st.range_x());
}
BENCHMARK_REGISTER_F(MyFixture, Bar)->Arg(42);


BENCHMARK_MAIN()
