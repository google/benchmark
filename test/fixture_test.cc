
#include "benchmark/benchmark.h"

#include <cassert>
#include <memory>
#include <map>
#include <cstring>

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


static int count = 0;
class CounterAccess : public ::benchmark::Fixture {
  public:

  void SetUp(benchmark::State &st) override {

    st.counters.skipZeroCounters = true;

    // add one counter at the beginning
    posFoo = st.counters.Insert("Foo");
    // add more counters to make lookup more expensive
    char buf[64];
    for(int i = 1; i < st.range_x() - 1; ++i) {
      // use a letter in between for the map comparison
      snprintf(buf, sizeof(buf), "CPad%d", i);
      st.counters.Insert(buf);
    }
    // add one counter at the end
    posBar = st.counters.Insert("Bar");

    count++;

    assert(posFoo != size_t(-1));
    assert(posBar != size_t(-1));
  }

  void TearDown(benchmark::State &st) override {
    assert(st.counters[posFoo].Value() > 0.);
    assert(st.counters[posBar].Value() > 0.);

    st.SetItemsProcessed(2 * st.iterations());
  }

  size_t posFoo = -1, posBar = -1;
};
BENCHMARK_DEFINE_F(CounterAccess, ById__)(benchmark::State& st) {
  while (st.KeepRunning()) {
    st.counters[posFoo] += 1.;
    st.counters[posBar] += 1.;
  }
  if(count == 26 || count == 25) {
    st.counters["Baz"] += 1000;
  }
}
BENCHMARK_DEFINE_F(CounterAccess, ByName)(benchmark::State& st) {
  while (st.KeepRunning()) {
    st.counters["Foo"] += 1.;
    st.counters["Bar"] += 1.;
  }
}
BENCHMARK_DEFINE_F(CounterAccess, VsMap_)(benchmark::State& st) {
  std::map< std::string, benchmark::Counter > map;
  for(auto const& c : st.counters) {
    map[c.Name()] = c;
  }
  while (st.KeepRunning()) {
    map["Foo"] += 1.;
    map["Bar"] += 1.;
  }
  st.counters["Foo"] = map["Foo"];
  st.counters["Bar"] = map["Bar"];
}

BENCHMARK_REGISTER_F(CounterAccess, ById__)->Arg(2);
BENCHMARK_REGISTER_F(CounterAccess, ByName)->Arg(2);
BENCHMARK_REGISTER_F(CounterAccess, VsMap_)->Arg(2);
BENCHMARK_REGISTER_F(CounterAccess, ById__)->Arg(16);
BENCHMARK_REGISTER_F(CounterAccess, ByName)->Arg(16);
BENCHMARK_REGISTER_F(CounterAccess, VsMap_)->Arg(16);
BENCHMARK_REGISTER_F(CounterAccess, ById__)->Arg(32);
BENCHMARK_REGISTER_F(CounterAccess, ByName)->Arg(32);
BENCHMARK_REGISTER_F(CounterAccess, VsMap_)->Arg(32);
BENCHMARK_REGISTER_F(CounterAccess, ById__)->Arg(64);
BENCHMARK_REGISTER_F(CounterAccess, ByName)->Arg(64);
BENCHMARK_REGISTER_F(CounterAccess, VsMap_)->Arg(64);
BENCHMARK_REGISTER_F(CounterAccess, ById__)->Arg(128);
BENCHMARK_REGISTER_F(CounterAccess, ByName)->Arg(128);
BENCHMARK_REGISTER_F(CounterAccess, VsMap_)->Arg(128);

BENCHMARK_MAIN()
