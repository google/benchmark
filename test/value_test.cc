#include "benchmark/benchmark.h"

#include <cassert>

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

BENCHMARK_MAIN()
