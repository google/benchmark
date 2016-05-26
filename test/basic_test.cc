
#include "benchmark/benchmark_api.h"

#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)->Arg(8)->Arg(512)->Arg(8192)

void BM_empty(benchmark::State& state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(state.iterations());
  }
}
BENCHMARK(BM_empty);
BENCHMARK(BM_empty)->ThreadPerCpu();

void BM_spin_empty(benchmark::State& state) {
  while (state.KeepRunning()) {
    for (int x = 0; x < state.range_x(); ++x) {
      benchmark::DoNotOptimize(x);
    }
  }
}
BASIC_BENCHMARK_TEST(BM_spin_empty);
BASIC_BENCHMARK_TEST(BM_spin_empty)->ThreadPerCpu();

void BM_spin_pause_before(benchmark::State& state) {
  for (int i = 0; i < state.range_x(); ++i) {
    benchmark::DoNotOptimize(i);
  }
  while(state.KeepRunning()) {
    for (int i = 0; i < state.range_x(); ++i) {
      benchmark::DoNotOptimize(i);
    }
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_before);
BASIC_BENCHMARK_TEST(BM_spin_pause_before)->ThreadPerCpu();


void BM_spin_pause_during(benchmark::State& state) {
  while(state.KeepRunning()) {
    state.PauseTiming();
    for (int i = 0; i < state.range_x(); ++i) {
      benchmark::DoNotOptimize(i);
    }
    state.ResumeTiming();
    for (int i = 0; i < state.range_x(); ++i) {
      benchmark::DoNotOptimize(i);
    }
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_during);
BASIC_BENCHMARK_TEST(BM_spin_pause_during)->ThreadPerCpu();

void BM_pause_during(benchmark::State& state) {
  while(state.KeepRunning()) {
    state.PauseTiming();
    state.ResumeTiming();
  }
}
BENCHMARK(BM_pause_during);
BENCHMARK(BM_pause_during)->ThreadPerCpu();
BENCHMARK(BM_pause_during)->UseRealTime();
BENCHMARK(BM_pause_during)->UseRealTime()->ThreadPerCpu();

void BM_spin_pause_after(benchmark::State& state) {
  while(state.KeepRunning()) {
    for (int i = 0; i < state.range_x(); ++i) {
      benchmark::DoNotOptimize(i);
    }
  }
  for (int i = 0; i < state.range_x(); ++i) {
    benchmark::DoNotOptimize(i);
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_after);
BASIC_BENCHMARK_TEST(BM_spin_pause_after)->ThreadPerCpu();


void BM_spin_pause_before_and_after(benchmark::State& state) {
  for (int i = 0; i < state.range_x(); ++i) {
    benchmark::DoNotOptimize(i);
  }
  while(state.KeepRunning()) {
    for (int i = 0; i < state.range_x(); ++i) {
      benchmark::DoNotOptimize(i);
    }
  }
  for (int i = 0; i < state.range_x(); ++i) {
    benchmark::DoNotOptimize(i);
  }
}
BASIC_BENCHMARK_TEST(BM_spin_pause_before_and_after);
BASIC_BENCHMARK_TEST(BM_spin_pause_before_and_after)->ThreadPerCpu();


void BM_empty_stop_start(benchmark::State& state) {
  while (state.KeepRunning()) { }
}
BENCHMARK(BM_empty_stop_start);
BENCHMARK(BM_empty_stop_start)->ThreadPerCpu();

#if __cplusplus >= 201103L

template <class ...Args>
void BM_with_args(benchmark::State& state, Args&&...) {
  while (state.KeepRunning()) {}
}
BENCHMARK_CAPTURE(BM_with_args, int_test, 42, 43, 44);
BENCHMARK_CAPTURE(BM_with_args, string_and_pair_test,
                  std::string("abc"), std::pair<int, double>(42, 3.8));

void BM_non_template_args(benchmark::State& state, int, double) {
  while(state.KeepRunning()) {}
}
BENCHMARK_CAPTURE(BM_non_template_args, basic_test, 0, 0);

#endif // __cplusplus >= 201103L

BENCHMARK_MAIN()
