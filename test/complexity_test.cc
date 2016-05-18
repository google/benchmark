
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
BENCHMARK(BM_Complexity_O1) -> Range(1, 1<<17) -> Complexity(benchmark::O_1);

static void BM_Complexity_O_N(benchmark::State& state) {
  auto v = ConstructRandomVector(state.range_x());
  const int itemNotInVector = state.range_x()*2; // Test worst case scenario (item not in vector)
  while (state.KeepRunning()) {
      benchmark::DoNotOptimize(std::find(v.begin(), v.end(), itemNotInVector));
  }
}
BENCHMARK(BM_Complexity_O_N) -> Range(1, 1<<10) -> Complexity(benchmark::O_N);
BENCHMARK(BM_Complexity_O_N) -> Range(1, 1<<10) -> Complexity(benchmark::O_Auto);

static void BM_Complexity_O_M_plus_N(benchmark::State& state) {
  std::string s1(state.range_x(), '-');
  std::string s2(state.range_x(), '-');
  while (state.KeepRunning())
    benchmark::DoNotOptimize(s1.compare(s2));
}
BENCHMARK(BM_Complexity_O_M_plus_N)
	->RangeMultiplier(2)->Range(1<<10, 1<<18) -> Complexity(benchmark::O_M_plus_N);
    
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
BENCHMARK(BM_Complexity_O_N_Squared) -> Range(1, 1<<8) -> Complexity(benchmark::O_N_Squared);
    
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
BENCHMARK(BM_Complexity_O_N_Cubed) -> DenseRange(1, 8) -> Complexity(benchmark::O_N_Cubed);

static void BM_Complexity_O_log_N(benchmark::State& state) {
  auto m = ConstructRandomMap(state.range_x());
  const int itemNotInVector = state.range_x()*2; // Test worst case scenario (item not in vector)
  while (state.KeepRunning()) {
      benchmark::DoNotOptimize(m.find(itemNotInVector));
  }
}
BENCHMARK(BM_Complexity_O_log_N) -> Range(1, 1<<10) -> Complexity(benchmark::O_log_N);

static void BM_Complexity_O_N_log_N(benchmark::State& state) {
  auto v = ConstructRandomVector(state.range_x());
  while (state.KeepRunning()) {
      std::sort(v.begin(), v.end());
  }
}
BENCHMARK(BM_Complexity_O_N_log_N) -> Range(1, 1<<16) -> Complexity(benchmark::O_N_log_N);
BENCHMARK(BM_Complexity_O_N_log_N) -> Range(1, 1<<16) -> Complexity(benchmark::O_Auto);

// Test benchmark with no range. Complexity is always calculated as O(1).
void BM_Extreme_Cases(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
}
BENCHMARK(BM_Extreme_Cases);
BENCHMARK(BM_Extreme_Cases)->Arg(42);

BENCHMARK_MAIN()