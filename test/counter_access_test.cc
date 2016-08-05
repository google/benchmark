#include <benchmark/benchmark.h>

#include <cassert>
#include <map>
#include <vector>
#include <cstring>
#include <cstdio>
#include <algorithm>


/** All the tests here lookup counters Foo and Bar inside the
 * KeepRunning() loop. These counters are padded in between
 * with a given number of other counters. */
class CounterAccess : public ::benchmark::Fixture {
public:

  void SetUp(benchmark::State &st) override {
    st.counters.skipZeroCounters = true;
    // add one counter at the beginning
    posFoo = st.counters.Insert("Foo");
    // add padding counters to make lookup more expensive
    char buf[64];
    for(int i = 1; i < st.range(0) - 1; ++i) {
      snprintf(buf, sizeof(buf), "Ct%d", i);
      st.counters.Insert(buf);
    }
    // add one counter at the end
    posBar = st.counters.Insert("Bar");
    assert(posFoo != size_t(-1));
    assert(posBar != size_t(-1));

    // create some accelerator structures for the tests.
    maps.clear();
    vecmaps.clear();
    vecmapc.clear();
    size_t id = 0;
    for(auto const& c : st.counters) {
      maps[c.Name()] = id++; // a std::map with name->id
    }
    for(auto const& c : maps) {
      // a flat name->id map implemented with std::string
      vecmaps.emplace_back(c.first, c.second);
      // a flat name->id map implemented with char[]
      vecmapc.emplace_back();
      auto &b = vecmapc.back();
      assert(strlen(c.first.c_str()) < sizeof(b.first));
      strcpy(b.first, c.first.c_str());
      b.second = c.second;
    }

    assert(maps.size() == st.counters.size());
    assert(vecmaps.size() == st.counters.size());
    assert(vecmapc.size() == st.counters.size());
  }

  void TearDown(benchmark::State &st) override {
    assert(st.counters[posFoo].Value() > 0.);
    assert(st.counters[posBar].Value() > 0.);
    st.SetItemsProcessed(2 * st.iterations());
  }

  size_t posFoo = size_t(-1), posBar = size_t(-1);

  //-----------------------------------
  // helper accelerator structures

  typedef char arrtype[16];
  typedef std::pair< std::string, size_t > vtypes;
  typedef std::pair< arrtype,     size_t > vtypec;

  std::map< std::string, size_t > maps; // a name->id map with std::string
  std::vector< vtypes > vecmaps; // a name->id flat map implemented with std::string
  std::vector< vtypec > vecmapc; // a name->id flat map implemented with char[]

  // to lookup vecmaps
  size_t flookups(const char *name) const {
    auto b = vecmaps.begin(), e = vecmaps.end();
    auto it = std::lower_bound(b, e, name, [](vtypes const& l, const char *r){
      return l.first < r;
    });
    assert(it != e);
    assert(it->first == name);
    return it->second;
  }

  // to lookup vecmapc
  size_t flookupc(const char *name) const {
    auto b = vecmapc.begin(), e = vecmapc.end();
    auto it = std::lower_bound(b, e, name, [](vtypec const& l, const char *r){
      return strcmp(l.first, r) < 0;
    });
    assert(it != e);
    assert(strcmp(it->first, name) == 0);
    return it->second;
  }

};

// Lookup Foo and Bar by id
BENCHMARK_DEFINE_F(CounterAccess, ById____)(benchmark::State& st) {
  while (st.KeepRunning()) {
    st.counters[posFoo] += 1.;
    st.counters[posBar] += 1.;
  }
}
// Lookup Foo and Bar by name
BENCHMARK_DEFINE_F(CounterAccess, ByName__)(benchmark::State& st) {
  while (st.KeepRunning()) {
    st.counters["Foo"] += 1.;
    st.counters["Bar"] += 1.;
  }
}
/** Lookup Foo and Bar by name, but on a std::map and not in the
 * counters object. This simulates the case that the counters are
 * stored in an std::map. */
BENCHMARK_DEFINE_F(CounterAccess, VsMap___)(benchmark::State& st) {
  while (st.KeepRunning()) {
    maps["Foo"] += 1;
    maps["Bar"] += 1;
  }
  st.counters["Foo"] = maps["Foo"];
  st.counters["Bar"] = maps["Bar"];
}

// Lookup Foo and Bar by name, but accelerated with a flat map of
// name(char[])->id map (ie an array map) lookup.
BENCHMARK_DEFINE_F(CounterAccess, NameIdFC)(benchmark::State& st) {
  while (st.KeepRunning()) {
    st.counters[flookupc("Foo")] += 1.;
    st.counters[flookupc("Bar")] += 1.;
  }
}
// Lookup Foo and Bar by name, but accelerated with a flat map of
// name(std::string)->id map (ie an array map) lookup.
BENCHMARK_DEFINE_F(CounterAccess, NameIdFS)(benchmark::State& st) {
  while (st.KeepRunning()) {
    st.counters[flookups("Foo")] += 1.;
    st.counters[flookups("Bar")] += 1.;
  }
}
// Lookup Foo and Bar by name, but accelerated with a std::map of
// name(std::string)->id lookup.
BENCHMARK_DEFINE_F(CounterAccess, NameIdMS)(benchmark::State& st) {
  while (st.KeepRunning()) {
    st.counters[maps["Foo"]] += 1.;
    st.counters[maps["Bar"]] += 1.;
  }
}

#define BMRF(arg) \
BENCHMARK_REGISTER_F(CounterAccess, ById____)->Arg(arg); \
BENCHMARK_REGISTER_F(CounterAccess, ByName__)->Arg(arg); \
BENCHMARK_REGISTER_F(CounterAccess, VsMap___)->Arg(arg); \
BENCHMARK_REGISTER_F(CounterAccess, NameIdFC)->Arg(arg); \
BENCHMARK_REGISTER_F(CounterAccess, NameIdFS)->Arg(arg); \
BENCHMARK_REGISTER_F(CounterAccess, NameIdMS)->Arg(arg);

BMRF(2)
BMRF(8)
BMRF(16)
BMRF(32)
BMRF(64)
BMRF(128)
BMRF(256)

BENCHMARK_MAIN()
