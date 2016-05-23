
#include "benchmark/benchmark_api.h"

#include <string>
#include <vector>
#include <map>
#include <algorithm>

std::vector<int> ConstructRandomVector(int size) {
  std::vector<int> v;
  v.reserve(size);
  for (int i = 0; i < size; ++i) {
    v.push_back(rand() % size);
  }
  return v;
}

std::map<int, int> ConstructRandomMap(int size) {
  std::map<int, int> m;
  for (int i = 0; i < size; ++i) {
    m.insert(std::make_pair(rand() % size, rand() % size));
  }
  return m;
}

void BM_Complexity_O1(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
}
BENCHMARK(BM_Complexity_O1) -> Range(1, 1<<18) -> Complexity(benchmark::o1);

static void BM_Complexity_O_N(benchmark::State& state) {
  auto v = ConstructRandomVector(state.range_x());
  const int itemNotInVector = state.range_x()*2; // Test worst case scenario (item not in vector)
  while (state.KeepRunning()) {
      benchmark::DoNotOptimize(std::find(v.begin(), v.end(), itemNotInVector));
  }
}
BENCHMARK(BM_Complexity_O_N) -> RangeMultiplier(2) -> Range(1<<10, 1<<16) -> Complexity(benchmark::oN);
BENCHMARK(BM_Complexity_O_N) -> RangeMultiplier(2) -> Range(1<<10, 1<<16) -> Complexity(benchmark::oAuto);
   
static void BM_Complexity_O_N_Squared(benchmark::State& state) {
  std::string s1(state.range_x(), '-');
  std::string s2(state.range_x(), '-');
  while (state.KeepRunning())
    for(char& c1 : s1) {
        for(char& c2 : s2) {
            benchmark::DoNotOptimize(c1 = 'a');
            benchmark::DoNotOptimize(c2 = 'b');
        }
    }
}
BENCHMARK(BM_Complexity_O_N_Squared) -> Range(1, 1<<8) -> Complexity(benchmark::oNSquared);
    
static void BM_Complexity_O_N_Cubed(benchmark::State& state) {
  std::string s1(state.range_x(), '-');
  std::string s2(state.range_x(), '-');
  std::string s3(state.range_x(), '-');
  while (state.KeepRunning())
    for(char& c1 : s1) {
        for(char& c2 : s2) {
            for(char& c3 : s3) {
                benchmark::DoNotOptimize(c1 = 'a');
                benchmark::DoNotOptimize(c2 = 'b');
                benchmark::DoNotOptimize(c3 = 'c');
            }
        }
    }
}
BENCHMARK(BM_Complexity_O_N_Cubed) -> DenseRange(1, 8) -> Complexity(benchmark::oNCubed);

static void BM_Complexity_O_log_N(benchmark::State& state) {
  auto m = ConstructRandomMap(state.range_x());
  const int itemNotInVector = state.range_x()*2; // Test worst case scenario (item not in vector)
  while (state.KeepRunning()) {
      benchmark::DoNotOptimize(m.find(itemNotInVector));
  }
}
BENCHMARK(BM_Complexity_O_log_N) 
    -> RangeMultiplier(2) -> Range(1<<10, 1<<16) -> Complexity(benchmark::oLogN);

static void BM_Complexity_O_N_log_N(benchmark::State& state) {
  auto v = ConstructRandomVector(state.range_x());
  while (state.KeepRunning()) {
      std::sort(v.begin(), v.end());
  }
}
BENCHMARK(BM_Complexity_O_N_log_N) -> RangeMultiplier(2) -> Range(1<<10, 1<<16) -> Complexity(benchmark::oNLogN);
BENCHMARK(BM_Complexity_O_N_log_N) -> RangeMultiplier(2) -> Range(1<<10, 1<<16) -> Complexity(benchmark::oAuto);

// Test benchmark with no range and check no complexity is calculated.
void BM_Extreme_Cases(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
}
BENCHMARK(BM_Extreme_Cases) -> Complexity(benchmark::oNLogN);
BENCHMARK(BM_Extreme_Cases) -> Arg(42) -> Complexity(benchmark::oAuto);

BENCHMARK_MAIN()