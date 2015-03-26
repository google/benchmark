
#include <cstddef>

#include "benchmark/benchmark_api.h"
#include "../src/walltime.h"

void BM_CPUTimeNow(benchmark::State& state) {
    while (state.KeepRunning()) {
        benchmark::WallTime volatile now = benchmark::walltime::CPUWalltimeNow();
        ((void)now);
    }
}
BENCHMARK(BM_CPUTimeNow);

void BM_ChronoTimeNow(benchmark::State& state) {
    while (state.KeepRunning()) {
        benchmark::WallTime volatile now = benchmark::walltime::ChronoWalltimeNow();
        ((void)now);
    }
}
BENCHMARK(BM_ChronoTimeNow);


BENCHMARK_MAIN()
