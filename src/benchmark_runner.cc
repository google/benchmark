// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "benchmark_runner.h"
#include "benchmark/benchmark.h"
#include "benchmark_api_internal.h"
#include "internal_macros.h"

#ifndef BENCHMARK_OS_WINDOWS
#ifndef BENCHMARK_OS_FUCHSIA
#include <sys/resource.h>
#endif
#include <sys/time.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "check.h"
#include "colorprint.h"
#include "commandlineflags.h"
#include "complexity.h"
#include "counter.h"
#include "internal_macros.h"
#include "log.h"
#include "mutex.h"
#include "re.h"
#include "statistics.h"
#include "string_util.h"
#include "thread_manager.h"
#include "thread_timer.h"

namespace benchmark {

namespace internal {

MemoryManager* memory_manager = nullptr;

namespace {

static const size_t kMaxIterations = 1000000000;

BenchmarkReporter::Run CreateRunReport(
    const benchmark::internal::BenchmarkInstance& b,
    const internal::ThreadManager::Result& results, size_t memory_iterations,
    const MemoryManager::Result& memory_result, double seconds) {
  // Create report about this benchmark run.
  BenchmarkReporter::Run report;

  report.run_name = b.name;
  report.error_occurred = results.has_error_;
  report.error_message = results.error_message_;
  report.report_label = results.report_label_;
  // This is the total iterations across all threads.
  report.iterations = results.iterations;
  report.time_unit = b.time_unit;

  if (!report.error_occurred) {
    if (b.use_manual_time) {
      report.real_accumulated_time = results.manual_time_used;
    } else {
      report.real_accumulated_time = results.real_time_used;
    }
    report.cpu_accumulated_time = results.cpu_time_used;
    report.complexity_n = results.complexity_n;
    report.complexity = b.complexity;
    report.complexity_lambda = b.complexity_lambda;
    report.statistics = b.statistics;
    report.counters = results.counters;

    if (memory_iterations > 0) {
      report.has_memory_result = true;
      report.allocs_per_iter =
          memory_iterations ? static_cast<double>(memory_result.num_allocs) /
                                  memory_iterations
                            : 0;
      report.max_bytes_used = memory_result.max_bytes_used;
    }

    internal::Finish(&report.counters, results.iterations, seconds, b.threads);
  }
  return report;
}

// Execute one thread of benchmark b for the specified number of iterations.
// Adds the stats collected for the thread into *total.
void RunInThread(const BenchmarkInstance* b, size_t iters, int thread_id,
                 ThreadManager* manager) {
  internal::ThreadTimer timer;
  State st = b->Run(iters, thread_id, &timer, manager);
  CHECK(st.iterations() >= st.max_iterations)
      << "Benchmark returned before State::KeepRunning() returned false!";
  {
    MutexLock l(manager->GetBenchmarkMutex());
    internal::ThreadManager::Result& results = manager->results;
    results.iterations += st.iterations();
    results.cpu_time_used += timer.cpu_time_used();
    results.real_time_used += timer.real_time_used();
    results.manual_time_used += timer.manual_time_used();
    results.complexity_n += st.complexity_length_n();
    internal::Increment(&results.counters, st.counters);
  }
  manager->NotifyThreadComplete();
}

struct AbstractIterationResults {
  std::vector<internal::ThreadManager::Result> results;

  bool has_error = false;
  size_t iters = 0;
  double real_time_used = 0.0;
  double seconds = 0.0;

  AbstractIterationResults() = default;

  virtual ~AbstractIterationResults() = default;

  // *Append* the new data.
  void append(internal::ThreadManager::Result result,
              const benchmark::internal::BenchmarkInstance& b) {
    has_error |= result.has_error_;
    iters += result.iterations;

    // Adjust real/manual time stats since they were reported per thread.
    result.real_time_used /= b.threads;
    result.manual_time_used /= b.threads;

    // So for how long were we running?
    real_time_used += result.real_time_used;
    // The benchmark may have requested manual or real time be used
    // in decision making. Respect that.
    if (b.use_manual_time) {
      seconds += result.manual_time_used;
    } else if (b.use_real_time) {
      seconds += result.real_time_used;
    } else {
      seconds += result.cpu_time_used;
    }

    results.emplace_back(result);
  }

  // This is the business logic. Implementations need to decide what this does.
  virtual void NewResult(internal::ThreadManager::Result result,
                         const benchmark::internal::BenchmarkInstance& b) = 0;
};

template <typename Derived, typename iteration_results>
class BenchmarkRunnerBase {
 public:
  using IterationResults = iteration_results;

  BenchmarkRunnerBase(const benchmark::internal::BenchmarkInstance& b_,
                      std::vector<BenchmarkReporter::Run>* complexity_reports_)
      : b(b_),
        complexity_reports(*complexity_reports_),
        min_time(!IsZero(b.min_time) ? b.min_time : FLAGS_benchmark_min_time),
        repeats(b.repetitions != 0 ? b.repetitions
                                   : FLAGS_benchmark_repetitions),
        has_explicit_iteration_count(b.iterations != 0),
        pool(b.threads - 1),
        predictedItersTotal(has_explicit_iteration_count ? b.iterations : 1) {
    run_results.display_report_aggregates_only =
        (FLAGS_benchmark_report_aggregates_only ||
         FLAGS_benchmark_display_aggregates_only);
    run_results.file_report_aggregates_only =
        FLAGS_benchmark_report_aggregates_only;
    if (b.aggregation_report_mode != internal::ARM_Unspecified) {
      run_results.display_report_aggregates_only =
          (b.aggregation_report_mode &
           internal::ARM_DisplayReportAggregatesOnly);
      run_results.file_report_aggregates_only =
          (b.aggregation_report_mode & internal::ARM_FileReportAggregatesOnly);
    }
  }

  Derived& getDerived() { return *static_cast<Derived*>(this); }

  void DoWork() {
    for (int repetition_num = 0; repetition_num < repeats; repetition_num++) {
      const bool is_the_first_repetition = repetition_num == 0;
      DoOneRepetition(is_the_first_repetition);
    }

    // Calculate additional statistics
    run_results.aggregates_only = ComputeStats(run_results.non_aggregates);

    // Maybe calculate complexity report
    if ((b.complexity != oNone) && b.last_benchmark_instance) {
      auto additional_run_stats = ComputeBigO(complexity_reports);
      run_results.aggregates_only.insert(run_results.aggregates_only.end(),
                                         additional_run_stats.begin(),
                                         additional_run_stats.end());
      complexity_reports.clear();
    }
  }

  RunResults&& get_results() {
    DoWork();
    return std::move(run_results);
  }

 private:
  RunResults run_results;

  const benchmark::internal::BenchmarkInstance& b;
  std::vector<BenchmarkReporter::Run>& complexity_reports;

  const double min_time;
  const int repeats;
  const bool has_explicit_iteration_count;

  std::vector<std::thread> pool;

  size_t predictedItersTotal;  // preserved between repetitions!
  // So only the first repetition has to find/calculate it,
  // the other repetitions will just use that precomputed iteration count.

 protected:
  IterationResults DoNIterations(size_t runForNumIters,
                                 const IterationResults& /*prev_iters*/) {
    // WARNING: runForNumIters and predictedItersTotal are not interchangeable!
    VLOG(2) << "Running " << b.name << " for " << runForNumIters << "\n";

    std::unique_ptr<internal::ThreadManager> manager;
    manager.reset(new internal::ThreadManager(b.threads));

    // Run all but one thread in separate threads
    for (std::size_t ti = 0; ti < pool.size(); ++ti) {
      pool[ti] = std::thread(&RunInThread, &b, runForNumIters,
                             static_cast<int>(ti + 1), manager.get());
    }
    // And run one thread here directly.
    // (If we were asked to run just one thread, we don't create new threads.)
    // Yes, we need to do this here *after* we start the separate threads.
    RunInThread(&b, runForNumIters, 0, manager.get());

    // The main thread has finished. Now let's wait for the other threads.
    manager->WaitForAllThreads();
    for (std::thread& thread : pool) thread.join();

    IterationResults i;
    // Acquire the measurements/counters from the manager, UNDER THE LOCK!
    {
      MutexLock l(manager->GetBenchmarkMutex());
      i.NewResult(manager->results, b);
    }

    // And get rid of the manager.
    manager.reset();

    VLOG(2) << "Ran in " << b.threads * i.results.back().cpu_time_used << "/"
            << b.threads * i.results.back().real_time_used << "\n";

    return i;
  }

 private:
  size_t PredictNumItersNeeded(const IterationResults& i) const {
    // See how much iterations should be increased by.
    // Note: Avoid division by zero with max(seconds, 1ns).
    double multiplier = min_time * 1.4 / std::max(i.seconds, 1e-9);
    // If our last run was at least 10% of FLAGS_benchmark_min_time then we
    // use the multiplier directly.
    // Otherwise we use at most 10 times expansion.
    // NOTE: When the last run was at least 10% of the min time the max
    // expansion should be 14x.
    bool is_significant = (i.seconds / min_time) > 0.1;
    multiplier = is_significant ? multiplier : std::min(10.0, multiplier);
    if (multiplier <= 1.0) multiplier = 2.0;

    // So what seems to be the sufficiently-large iteration count? Round up.
    const size_t max_next_iters =
        0.5 + std::max(multiplier * i.iters, i.iters + 1.0);
    // But we do have *some* sanity limits though..
    const size_t next_iters = std::min(max_next_iters, kMaxIterations);

    VLOG(3) << "Next iters: " << next_iters << ", " << multiplier << "\n";
    return next_iters;  // round up before conversion to integer.
  }

  bool ShouldReportIterationResults(const IterationResults& i) const {
    // Determine if this run should be reported;
    // Either it has run for a sufficient amount of time
    // or because an error was reported.
    return i.has_error ||
           i.iters >= kMaxIterations ||  // Too many iterations already.
           i.seconds >= min_time ||      // The elapsed time is large enough.
           // CPU time is specified but the elapsed real time greatly exceeds
           // the minimum time.
           // Note that user provided timers are except from this sanity check.
           ((i.real_time_used >= 5 * min_time) && !b.use_manual_time);
  }

 protected:
  void DoOneRepetition(bool is_the_first_repetition) {
    IterationResults i;

    // We *may* be gradually increasing the length (iteration count)
    // of the benchmark until we decide the results are significant.
    // And once we do, we report those last results and exit.
    // Please do note that the if there are repetitions, the iteration count
    // is *only* calculated for the *first* repetition, and other repetitions
    // simply use that precomputed iteration count.
    for (;;) {
      i = getDerived().DoNIterations(predictedItersTotal, i);

      // Do we consider the results to be significant?
      // If we are doing repetitions, and the first repetition was already done,
      // it has calculated the correct iteration time, so we have run that very
      // iteration count just now. No need to calculate anything. Just report.
      // Else, the normal rules apply.
      const bool results_are_significant = !is_the_first_repetition ||
                                           has_explicit_iteration_count ||
                                           ShouldReportIterationResults(i);

      if (results_are_significant) break;  // Good, let's report them!

      // Nope, bad iteration. Let's re-estimate the hopefully-sufficient
      // iteration count, and run the benchmark again...

      predictedItersTotal = PredictNumItersNeeded(i);
      assert(predictedItersTotal > i.iters &&
             "if we did more iterations than we want to do the next time, "
             "then we should have accepted the current iteration run.");
    }

    // Oh, one last thing, we need to also produce the 'memory measurements'..
    MemoryManager::Result memory_result;
    size_t memory_iterations = 0;
    if (memory_manager != nullptr) {
      // Only run a few iterations to reduce the impact of one-time
      // allocations in benchmarks that are not properly managed.
      memory_iterations = std::min<size_t>(16, predictedItersTotal);
      memory_manager->Start();
      std::unique_ptr<internal::ThreadManager> manager;
      manager.reset(new internal::ThreadManager(1));
      RunInThread(&b, memory_iterations, 0, manager.get());
      manager->WaitForAllThreads();
      manager.reset();

      memory_manager->Stop(&memory_result);
    }

    for (const internal::ThreadManager::Result& result : i.results) {
      // Ok, now actualy report.
      BenchmarkReporter::Run report = CreateRunReport(
          b, result, memory_iterations, memory_result, i.seconds);

      if (!report.error_occurred && b.complexity != oNone)
        complexity_reports.push_back(report);

      run_results.non_aggregates.push_back(report);
    }
  }
};

struct IterationAveragedResults final : public AbstractIterationResults {
  // Just replace all of the existing data.
  void NewResult(internal::ThreadManager::Result result,
                 const benchmark::internal::BenchmarkInstance& b) final {
    IterationAveragedResults empty;
    empty.append(result, b);
    *this = std::move(empty);
  }
};

class AveragingBenchmarkRunner final
    : public BenchmarkRunnerBase<AveragingBenchmarkRunner,
                                 IterationAveragedResults> {
 public:
  AveragingBenchmarkRunner(
      const benchmark::internal::BenchmarkInstance& b_,
      std::vector<BenchmarkReporter::Run>* complexity_reports_)
      : BenchmarkRunnerBase(b_, complexity_reports_) {}
};

}  // end namespace

RunResults RunBenchmark(
    const benchmark::internal::BenchmarkInstance& b,
    std::vector<BenchmarkReporter::Run>* complexity_reports) {
  internal::AveragingBenchmarkRunner r(b, complexity_reports);
  return r.get_results();
}

}  // end namespace internal

}  // end namespace benchmark
