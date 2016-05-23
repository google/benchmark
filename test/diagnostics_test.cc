
#include "benchmark/benchmark_api.h"
#include "../src/check.h"

void test_handler() {
  throw std::logic_error("");
}

void try_invalid_pause_resume(benchmark::State& state) {
#ifndef NDEBUG
  try {
    state.PauseTiming();
    std::abort();
  } catch (std::logic_error const&) {}
  try {
    state.ResumeTiming();
    std::abort();
  } catch (std::logic_error const&) {}
#else
  (void)state; // avoid unused warning
#endif
}

void BM_diagnostic_test(benchmark::State& state) {
  static bool called_once = false;

  if (called_once == false) try_invalid_pause_resume(state);

  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(state.iterations());
  }

  if (called_once == false) try_invalid_pause_resume(state);

  called_once = true;
}
BENCHMARK(BM_diagnostic_test);

int main(int argc, char** argv) {
  benchmark::internal::get_abort_handler() = &test_handler;
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
}
