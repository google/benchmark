
#include "benchmark/benchmark.h"

#include <map>
#include <memory>


class EmptyMapFixture : public ::benchmark::Fixture {
  public:
    std::map<int, int> m;
};

class MapFixture : public EmptyMapFixture {
 public:
  void SetUp(const ::benchmark::State& st) {
    const int size = st.range_x();
    for (int i = 0; i < size; ++i) {
      m.insert(std::make_pair(rand() % size, rand() % size));
    }
  }

  void TearDown() {
    m.clear();
  }
};

BENCHMARK_F(EmptyMapFixture, Insert)(benchmark::State& state) {
  int i = 0;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(m.insert(std::make_pair(i, i)));
    ++i;
  }
  state.SetBytesProcessed(state.iterations() * sizeof(int));
}

BENCHMARK_DEFINE_F(MapFixture, Lookup)(benchmark::State& state) {
  const int size = state.range_x();
  while (state.KeepRunning()) {
    for (int i = 0; i < size; ++i) {
      benchmark::DoNotOptimize(m.find(rand() % size));
    }
  }
  state.SetItemsProcessed(state.iterations() * size);
  state.SetBytesProcessed(state.iterations() * size * sizeof(int));
}
BENCHMARK_REGISTER_F(MapFixture, Lookup)->Range(1<<3, 1<<12);

BENCHMARK_MAIN()
