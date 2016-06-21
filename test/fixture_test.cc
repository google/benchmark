
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

  void InitState(benchmark::State &st) override {
      posFoo = st.AddCounter("Foo");
      posBar = st.AddCounter("Bar");
  }

  size_t posFoo = -1, posBar = -1;
};
BENCHMARK_DEFINE_F(CounterFixture, BumpById)(benchmark::State& st) {
  assert(posFoo == 0);
  assert(posBar == 1);
  while (st.KeepRunning()) {
      st.GetCounter(posFoo).value += 1.f;
      st.GetCounter(posBar).value += 1.f;
  }
  assert(st.GetCounter(posFoo).value != 0.);
  st.SetItemsProcessed(st.iterations());
}
BENCHMARK_REGISTER_F(CounterFixture, BumpById);

BENCHMARK_DEFINE_F(CounterFixture, BumpByName)(benchmark::State& st) {
  while (st.KeepRunning()) {
      st.GetCounter("Foo").value += 1.f;
      st.GetCounter("Bar").value += 1.f;
  }
  st.SetItemsProcessed(st.iterations());
}
BENCHMARK_REGISTER_F(CounterFixture, BumpByName);

BENCHMARK_MAIN()
