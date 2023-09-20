
#undef NDEBUG

#include <chrono>
#include <thread>
#include <future>

#include "../src/timers.h"
#include "benchmark/benchmark.h"
#include "output_test.h"

namespace {

static const std::chrono::duration<double, std::milli> time_frame(50);
static const double time_frame_in_sec(
    std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(
        time_frame)
        .count());

void MyBusySpinwait() {
  const auto start = benchmark::ChronoClockNow();

  while (true) {
    const auto now = benchmark::ChronoClockNow();
    const auto elapsed = now - start;

    if (std::chrono::duration<double, std::chrono::seconds::period>(elapsed) >=
        time_frame)
      return;
  }
}

}

// ========================================================================= //
// --------------------------- TEST CASES BEGIN ---------------------------- //
// ========================================================================= //

// ========================================================================= //
// BM_ManualThreadingInLoop
// Measurements include the creation and joining of threads.

void BM_ManualThreadingInLoop(benchmark::State& state) {
  int numWorkerThreads = state.threads() - 1;
  std::vector<std::thread> pool (numWorkerThreads);

  for (auto _ : state) {

    for (int i = 0; i < numWorkerThreads; ++i)
    {
      pool[i] = std::thread(MyBusySpinwait);
    }
    MyBusySpinwait();
    for (int i = 0; i < numWorkerThreads; ++i)
    {
      pool[i].join();
    }
    state.SetIterationTime(time_frame_in_sec);
  }
  state.counters["invtime"] =
      benchmark::Counter{1, benchmark::Counter::kIsRate};
}

BENCHMARK(BM_ManualThreadingInLoop)->Iterations(1)->ManualThreading()->Threads(1);
BENCHMARK(BM_ManualThreadingInLoop)->Iterations(1)->ManualThreading()->Threads(1)->UseRealTime();
BENCHMARK(BM_ManualThreadingInLoop)->Iterations(1)->ManualThreading()->Threads(1)->UseManualTime();
BENCHMARK(BM_ManualThreadingInLoop)->Iterations(1)->ManualThreading()->Threads(1)->MeasureProcessCPUTime();
BENCHMARK(BM_ManualThreadingInLoop)
    ->Iterations(1)
    ->ManualThreading()
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_ManualThreadingInLoop)
    ->Iterations(1)
    ->ManualThreading()
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

BENCHMARK(BM_ManualThreadingInLoop)->Iterations(1)->ManualThreading()->Threads(2);
BENCHMARK(BM_ManualThreadingInLoop)->Iterations(1)->ManualThreading()->Threads(2)->UseRealTime();
BENCHMARK(BM_ManualThreadingInLoop)->Iterations(1)->ManualThreading()->Threads(2)->UseManualTime();
BENCHMARK(BM_ManualThreadingInLoop)->Iterations(1)->ManualThreading()->Threads(2)->MeasureProcessCPUTime();
BENCHMARK(BM_ManualThreadingInLoop)
    ->Iterations(1)
    ->ManualThreading()
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_ManualThreadingInLoop)
    ->Iterations(1)
    ->ManualThreading()
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

// ========================================================================= //
// BM_ManualThreadingBeforeLoop
// Creation of threads is done before the start of the measurement, joining after the finish of the measurement.

void BM_ManualThreadingBeforeLoop(benchmark::State& state) {

  std::promise<void> thread_starter;
  auto starter_future = thread_starter.get_future();

  auto threadedLoop = [&]() {
    starter_future.wait();
    benchmark::ThreadState ts(state);
    for (auto _ : ts) {
      MyBusySpinwait();
      ts.SetIterationTime(time_frame_in_sec);
    }
  };

  std::vector<std::thread> pool (state.threads());
  for (int i = 0; i < state.threads(); ++i)
  {
    pool[i] = std::thread(threadedLoop);
  }
  thread_starter.set_value();
  for (int i = 0; i < state.threads(); ++i)
  {
    pool[i].join();
  }

  state.counters["invtime"] =
      benchmark::Counter{1, benchmark::Counter::kIsRate};
}

BENCHMARK(BM_ManualThreadingBeforeLoop)->Iterations(1)->ManualThreading()->Threads(1);
BENCHMARK(BM_ManualThreadingBeforeLoop)->Iterations(1)->ManualThreading()->Threads(1)->UseRealTime();
BENCHMARK(BM_ManualThreadingBeforeLoop)->Iterations(1)->ManualThreading()->Threads(1)->UseManualTime();
BENCHMARK(BM_ManualThreadingBeforeLoop)->Iterations(1)->ManualThreading()->Threads(1)->MeasureProcessCPUTime();
BENCHMARK(BM_ManualThreadingBeforeLoop)
    ->Iterations(1)
    ->ManualThreading()
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_ManualThreadingBeforeLoop)
    ->Iterations(1)
    ->ManualThreading()
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

BENCHMARK(BM_ManualThreadingBeforeLoop)->Iterations(1)->ManualThreading()->Threads(2);
BENCHMARK(BM_ManualThreadingBeforeLoop)->Iterations(1)->ManualThreading()->Threads(2)->UseRealTime();
BENCHMARK(BM_ManualThreadingBeforeLoop)->Iterations(1)->ManualThreading()->Threads(2)->UseManualTime();
BENCHMARK(BM_ManualThreadingBeforeLoop)->Iterations(1)->ManualThreading()->Threads(2)->MeasureProcessCPUTime();
BENCHMARK(BM_ManualThreadingBeforeLoop)
    ->Iterations(1)
    ->ManualThreading()
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_ManualThreadingBeforeLoop)
    ->Iterations(1)
    ->ManualThreading()
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

// ========================================================================= //
// ---------------------------- TEST CASES END ----------------------------- //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
