
#include "benchmark/benchmark.h"

#include <cassert>
#include <memory>

class MyFixture : public ::benchmark::Fixture {
 public:
  void SetUp(const ::benchmark::State& state) {
    if (state.thread_index == 0) {
      assert(data.get() == nullptr);
      data.reset(new int(42));
    }
  }

  void TearDown(const ::benchmark::State& state) {
    if (state.thread_index == 0) {
      assert(data.get() != nullptr);
      data.reset();
    }
  }

  ~MyFixture() {
    assert(data == nullptr);
  }

  std::unique_ptr<int> data;
};


BENCHMARK_F(MyFixture, Foo)(benchmark::State& st) {
  assert(data.get() != nullptr);
  assert(*data == 42);
  while (st.KeepRunning()) {
  }
}

BENCHMARK_DEFINE_F(MyFixture, Bar)(benchmark::State& st) {
  if (st.thread_index == 0) {
    assert(data.get() != nullptr);
    assert(*data == 42);
  }
  while (st.KeepRunning()) {
    assert(data.get() != nullptr);
    assert(*data == 42);
  }
  st.SetItemsProcessed(st.range_x());
}
BENCHMARK_REGISTER_F(MyFixture, Bar)->Arg(42);
BENCHMARK_REGISTER_F(MyFixture, Bar)->Arg(42)->ThreadPerCpu();


class CounterFixture : public ::benchmark::Fixture {
  public:

  void InitState(benchmark::State &st) {
      posFoo = st.counters.Set("Foo");
      posBar = st.counters.Set("Bar");
  }

  size_t posFoo, posBar;
};
BENCHMARK_DEFINE_F(CounterFixture, BumpById)(benchmark::State& st) {
  assert(posFoo == 0);
  assert(posBar == 1);
  while (st.KeepRunning()) {
      st.counters.Get(posFoo) += 1.;
      st.counters.Get(posBar) += 1.;
  }
  assert(st.GetCounter(posFoo).Value() > 0.);
  st.SetItemsProcessed(st.iterations());
}
BENCHMARK_REGISTER_F(CounterFixture, BumpById);

BENCHMARK_DEFINE_F(CounterFixture, BumpByName)(benchmark::State& st) {
  while (st.KeepRunning()) {
      st.counters.Get("Foo") += 1.;
      st.counters.Get("Bar") += 1.;
  }
  assert(st.GetCounter(posFoo).Value() > 0.);
  st.SetItemsProcessed(st.iterations());
}
BENCHMARK_REGISTER_F(CounterFixture, BumpByName);

BENCHMARK_MAIN()
