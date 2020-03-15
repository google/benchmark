
#undef NDEBUG

#include <chrono>
#include <thread>
#include "../src/timers.h"
#include "../src/check.h"
#include "benchmark/benchmark.h"

static const std::chrono::duration<double, std::milli> time_frame(50);
static const double time_frame_in_ns(
    std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(
        time_frame)
        .count());
static const double time_frame_in_sec(
    std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(
        time_frame)
        .count());

static const int nr_iterations = 10;

// Waste exact amount of CPU time in busy-loop
void MyBusySpinwait() {
  const auto start = benchmark::ThreadCPUUsage();

  while (true) {
    const auto now = benchmark::ThreadCPUUsage();
    const auto elapsed = now - start;

    if (std::chrono::duration<double, std::chrono::seconds::period>(elapsed) >=
        time_frame)
      return;
  }
}

class TestReporter : public benchmark::ConsoleReporter {
  int num_cpus = 0;
 public:
  virtual bool ReportContext(const Context& context) {
    num_cpus = context.cpu_info.num_cpus;
    return ConsoleReporter::ReportContext(context);
  };

  virtual void ReportRuns(const std::vector<Run>& report) {
    ConsoleReporter::ReportRuns(report);

    for (auto &run: report) {
      double expected_cpus = 1;
      int64_t min_cpus = run.threads;

      if (run.run_name.function_name == "BM_WorkerThread") {
        if (run.run_name.time_type == "" ||
            run.run_name.time_type == "real_time" ||
            run.run_name.time_type == "manual_time") {
          expected_cpus = 0;
        }
      }

      if (run.run_name.function_name == "BM_MainThreadAndWorkerThread") {
        min_cpus *= 2;
        if (run.run_name.time_type == "process_time" ||
            run.run_name.time_type == "process_time/real_time" ||
            run.run_name.time_type == "process_time/manual_time") {
          expected_cpus = 2;
        }
      }

      if (run.run_name.time_type == "manual_time" ||
          run.run_name.time_type == "process_time/manual_time") {
        min_cpus = 0;
      }

      double cpus = run.GetAdjustedCPUTime() / time_frame_in_ns;
      double real = run.GetAdjustedRealTime() / time_frame_in_ns;

      // Check that result >= expected (accuracy: relative=0.1, absolute=0.1)
      CHECK_FLOAT_GE(cpus, expected_cpus, expected_cpus * 0.1 + 0.1);
      CHECK_FLOAT_GE(real, 1.0, 0.2);

      // Warn if cpu time is bigger than expected.
      if (cpus > expected_cpus * 1.1 + 0.1) {
        VLOG(0) << "CPU time bigger than expected, might be cpu overload\n";
      }

      // Warn if real time is bigger than expected.
      if (real > 1.2) {
        VLOG(0) << "Real time bigger than expected, might be cpu overload\n";
      }

      // Check that cpu time <= expected * 2 + 20%
      CHECK_FLOAT_LE(cpus, expected_cpus, expected_cpus + 0.2);

      // For checking real time require one more CPU for infrastructure
      if (num_cpus < min_cpus + 1) {
        VLOG(0) << "Not enough cpus to get valid real time\n";
      } else {
        // Measurements in CI are noisy, check real time <= expected * 4 + 20%
        CHECK_FLOAT_LE(real, 4.0, 0.2);
      }
    }
  }

  TestReporter() {}
  virtual ~TestReporter() {}
};

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

BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(1);
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->UseRealTime();
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->UseManualTime();
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->MeasureProcessCPUTime();
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(2);
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->UseRealTime();
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->UseManualTime();
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->MeasureProcessCPUTime();
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_MainThread)
    ->Iterations(nr_iterations)
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

BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1);
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->UseRealTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->UseManualTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->MeasureProcessCPUTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2);
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->UseRealTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->UseManualTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->MeasureProcessCPUTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_WorkerThread)
    ->Iterations(nr_iterations)
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

BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1);
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->UseRealTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->UseManualTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->MeasureProcessCPUTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(1)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2);
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->UseRealTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->UseManualTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->MeasureProcessCPUTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(nr_iterations)
    ->Threads(2)
    ->MeasureProcessCPUTime()
    ->UseManualTime();

// ========================================================================= //
// ---------------------------- TEST CASES END ----------------------------- //
// ========================================================================= //

int main(int argc, char* argv[]) {
  benchmark::Initialize(&argc, argv);
  TestReporter test_reporter;
  benchmark::RunSpecifiedBenchmarks(&test_reporter);
  return 0;
}
