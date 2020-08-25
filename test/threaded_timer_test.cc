
#undef NDEBUG

#include <chrono>
#include <thread>
#include "benchmark/benchmark.h"
#include "output_test.h"
#include "spinning.h"

// ========================================================================= //
// --------------------------- TEST CASES BEGIN ---------------------------- //
// ========================================================================= //

// ========================================================================= //
// BM_MainThread

void BM_UnbalancedThreads(benchmark::State& state) {
  const std::chrono::duration<double, std::milli> tf(
    state.thread_index == 0 ? 100 : 20);
  const double tf_in_sec(
    std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(
        tf).count());

  for (auto _ : state) {
    TimedBusySpinwait(tf);
    state.SetIterationTime(tf_in_sec);
  }
}

BENCHMARK(BM_UnbalancedThreads)->Iterations(1)->ThreadRange(1, 4);
BENCHMARK(BM_UnbalancedThreads)->Iterations(1)->ThreadRange(1, 4)->UseRealTime();

void CheckTimings(Results const& e) {
  // check that the real time is between 100 and 110 ms
  CHECK_FLOAT_RESULT_VALUE(e, "real_time", GE, 1e08, 0.01);
  CHECK_FLOAT_RESULT_VALUE(e, "real_time", LT, 1.1e08, 0.01);

  // check that the cpu time is in an reasonable interval
  double min_cpu_time = ((e.NumThreads() - 1) * 20 + 100) * 1e06;
  double max_cpu_time = ((e.NumThreads() - 1) * 30 + 110) * 1e06;
  CHECK_FLOAT_RESULT_VALUE(e, "cpu_time", GE, min_cpu_time, 0.01);
  CHECK_FLOAT_RESULT_VALUE(e, "cpu_time", LT, max_cpu_time, 0.01);
}

CHECK_BENCHMARK_RESULTS("BM_UnbalancedThreads", &CheckTimings);

// ========================================================================= //
// ---------------------------- TEST CASES END ----------------------------- //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
