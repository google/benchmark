#include "benchmark/benchmark.h"

#include <cassert>
#include <algorithm>
#include <vector>

void BM_one(benchmark::State& st, int x) {
    assert(x == 42);
    ((void)x);
    while (st.KeepRunning()) {}
}
BENCHMARK_V(BM_one, 42);


void BM_two(benchmark::State& st, int x, int y) {
    assert(x == 42);
    ((void)x);
    assert(x +1 == y);
    ((void)y);
    while (st.KeepRunning()) {}
}
BENCHMARK_V(BM_two, 42, 43);

void BM_vector_find(benchmark::State& st, std::vector<int> const& v, int x) {
    while (st.KeepRunning()) {
        benchmark::DoNotOptimize(std::find(v.begin(), v.end(), x));
    }
}
BENCHMARK_V(BM_vector_find, std::vector<int>({1, 2, 3, 4}), 5);

BENCHMARK_MAIN()
