
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

void BM_MainThread(benchmark::State& state) {
  for (auto _ : state) {
    MyBusySpinwait();
    state.SetIterationTime(time_frame_in_sec);
  }
  state.counters["invtime"] =
      benchmark::Counter{1, benchmark::Counter::kIsRate};
}

BENCHMARK(BM_MainThread)->Iterations(1)->Threads(1);
BENCHMARK(BM_MainThread)->Iterations(1)->Threads(1)->UseRealTime();
BENCHMARK(BM_MainThread)->Iterations(1)->Threads(1)->UseManualTime();
BENCHMARK(BM_MainThread)->Iterations(1)->Threads(1)->MeasureProcessCPUTime();
BENCHMARK(BM_MainThread)
    ->Iterations(1)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_MainThread)
    ->Iterations(1)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

BENCHMARK(BM_MainThread)->Iterations(1)->Threads(2);
BENCHMARK(BM_MainThread)->Iterations(1)->Threads(2)->UseRealTime();
BENCHMARK(BM_MainThread)->Iterations(1)->Threads(2)->UseManualTime();
BENCHMARK(BM_MainThread)->Iterations(1)->Threads(2)->MeasureProcessCPUTime();
BENCHMARK(BM_MainThread)
    ->Iterations(1)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_MainThread)
    ->Iterations(1)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

// ========================================================================= //
// BM_WorkerThread

void BM_WorkerThread(benchmark::State& state) {
  for (auto _ : state) {
    std::thread Worker(&MyBusySpinwait);
    Worker.join();
    state.SetIterationTime(time_frame_in_sec);
  }
  state.counters["invtime"] =
      benchmark::Counter{1, benchmark::Counter::kIsRate};
}

BENCHMARK(BM_WorkerThread)->Iterations(1)->Threads(1);
BENCHMARK(BM_WorkerThread)->Iterations(1)->Threads(1)->UseRealTime();
BENCHMARK(BM_WorkerThread)->Iterations(1)->Threads(1)->UseManualTime();
BENCHMARK(BM_WorkerThread)->Iterations(1)->Threads(1)->MeasureProcessCPUTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(1)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(1)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

BENCHMARK(BM_WorkerThread)->Iterations(1)->Threads(2);
BENCHMARK(BM_WorkerThread)->Iterations(1)->Threads(2)->UseRealTime();
BENCHMARK(BM_WorkerThread)->Iterations(1)->Threads(2)->UseManualTime();
BENCHMARK(BM_WorkerThread)->Iterations(1)->Threads(2)->MeasureProcessCPUTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(1)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(1)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

// ========================================================================= //
// BM_MainThreadAndWorkerThread

void BM_MainThreadAndWorkerThread(benchmark::State& state) {
  for (auto _ : state) {
    std::thread Worker(&MyBusySpinwait);
    MyBusySpinwait();
    Worker.join();
    state.SetIterationTime(time_frame_in_sec);
  }
  state.counters["invtime"] =
      benchmark::Counter{1, benchmark::Counter::kIsRate};
}

BENCHMARK(BM_MainThreadAndWorkerThread)->Iterations(1)->Threads(1);
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(1)
    ->UseRealTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(1)
    ->UseManualTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(1)
    ->MeasureProcessCPUTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

BENCHMARK(BM_MainThreadAndWorkerThread)->Iterations(1)->Threads(2);
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(2)
    ->UseRealTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(2)
    ->UseManualTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(2)
    ->MeasureProcessCPUTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

// ========================================================================= //
// ---------------------------- TEST CASES END ----------------------------- //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
