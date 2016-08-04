
#include "benchmark/benchmark.h"

#include <cassert>
#include <memory>

class MyFixture : public ::benchmark::Fixture {
 public:
  void SetUp(::benchmark::State& state) {
    if (state.thread_index == 0) {
      assert(data.get() == nullptr);
      data.reset(new int(42));
    }
  }

  void TearDown(::benchmark::State& state) {
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

  void SetUp(benchmark::State &st) override {
    // add one counter at the beginning
    posFoo = st.counters.Insert("Foo");
    // add more counters to make lookup more expensive
    char buf[64];
    for(int i = 1; i < st.range_x() - 1; ++i) {
      snprintf(buf, sizeof(buf), "Pad%d", i);
      st.counters.Insert(buf);
    }
    // add one counter at the end
    posBar = st.counters.Insert("Bar");
    st.counters.Insert("Baz");
  }

  size_t posFoo = -1, posBar = -1;
};
BENCHMARK_DEFINE_F(CounterFixture, BumpById)(benchmark::State& st) {
  assert(posFoo != size_t(-1));
  assert(posBar != size_t(-1));
  while (st.KeepRunning()) {
    st.counters[posFoo] += 1.;
    st.counters[posBar] += 1.;
  }
  assert(st.counters[posFoo].Value() > 0.);
  assert(st.counters[posBar].Value() > 0.);
  st.SetItemsProcessed(st.iterations());
}
BENCHMARK_DEFINE_F(CounterFixture, BumpByName)(benchmark::State& st) {
  assert(posFoo != size_t(-1));
  assert(posBar != size_t(-1));
  while (st.KeepRunning()) {
    st.counters["Foo"] += 1.;
    st.counters["Bar"] += 1.;
  }
  assert(st.counters[posFoo].Value() > 0.);
  assert(st.counters[posBar].Value() > 0.);
  st.SetItemsProcessed(st.iterations());
}

BENCHMARK_REGISTER_F(CounterFixture, BumpById)->Arg(2);
BENCHMARK_REGISTER_F(CounterFixture, BumpByName)->Arg(2);
BENCHMARK_REGISTER_F(CounterFixture, BumpById)->Arg(16);
BENCHMARK_REGISTER_F(CounterFixture, BumpByName)->Arg(16);
BENCHMARK_REGISTER_F(CounterFixture, BumpById)->Arg(64);
BENCHMARK_REGISTER_F(CounterFixture, BumpByName)->Arg(64);

BENCHMARK_MAIN()
