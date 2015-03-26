
#include <cstddef>

#include "benchmark/benchmark_api.h"
#include "../src/walltime.h"

void BM_CPUTimeNow(benchmark::State& state) {
    using benchmark::walltime::CPUWalltimeNow;
    while (state.KeepRunning()) {
        benchmark::WallTime volatile now;
        now = CPUWalltimeNow();
        now = CPUWalltimeNow();
        now = CPUWalltimeNow();
        now = CPUWalltimeNow();
        now = CPUWalltimeNow();
        ((void)now);
    }
}
BENCHMARK(BM_CPUTimeNow);

void BM_ChronoTimeNow(benchmark::State& state) {
    using benchmark::walltime::ChronoWalltimeNow;
    while (state.KeepRunning()) {
        benchmark::WallTime volatile now;
        now = ChronoWalltimeNow();
        now = ChronoWalltimeNow();
        now = ChronoWalltimeNow();
        now = ChronoWalltimeNow();
        now = ChronoWalltimeNow();
        ((void)now);
    }
}
BENCHMARK(BM_ChronoTimeNow);


BENCHMARK_MAIN()
