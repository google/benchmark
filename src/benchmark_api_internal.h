#ifndef BENCHMARK_API_INTERNAL_H
#define BENCHMARK_API_INTERNAL_H

#include "benchmark/benchmark.h"
#include "commandlineflags.h"

#include <chrono>
#include <cmath>
#include <iosfwd>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace benchmark {
namespace internal {

extern const double kSafetyMultiplier;

// Information kept per benchmark we may want to run
class BenchmarkInstance {
 public:
  BenchmarkInstance(Benchmark* benchmark, const std::vector<int64_t>& args,
                    int threads);

  // Const accessors.

  const BenchmarkName& name() const;
  int64_t repetitions() const;
  const std::vector<Statistics>* statistics() const;
  AggregationReportMode aggregation_report_mode() const;
  TimeUnit time_unit() const;
  int threads() const;
  bool measure_process_cpu_time() const;
  bool use_real_time() const;
  bool use_manual_time() const;
  BigO complexity() const;
  BigOFunc* complexity_lambda() const;
  bool last_benchmark_instance() const;
  IterationCount iterations() const;

  // Returns the min time to run a microbenchmark in RunBenchmark().
  double min_time() const;

  // Returns number of repetitions for Random Interleaving. This will be
  // initialized later once we finish the first repetition, if Random
  // Interleaving is enabled. See also ComputeRandominterleavingrepetitions().
  int64_t random_interleaving_repetitions() const;

  // Returns true if repetitions for Random Interleaving is initialized.
  bool random_interleaving_repetitions_initialized() const;

  // Initializes number of repetitions for random interleaving.
  void init_random_interleaving_repetitions(int64_t repetitions) const;

  // Setters.

  // Sets the value of last_benchmark_instance.
  void set_last_benchmark_instance(bool last_benchmark_instance) {
    last_benchmark_instance_ = last_benchmark_instance;
  }

  // Public APIs.

  State Run(IterationCount iters, int thread_id, internal::ThreadTimer* timer,
            internal::ThreadManager* manager,
            internal::PerfCountersMeasurement* perf_counters_measurement) const;

 private:
  BenchmarkName name_;
  Benchmark* benchmark_;
  AggregationReportMode aggregation_report_mode_;
  std::vector<int64_t> args_;
  TimeUnit time_unit_;
  int range_multiplier_;
  bool measure_process_cpu_time_;
  bool use_real_time_;
  bool use_manual_time_;
  BigO complexity_;
  BigOFunc* complexity_lambda_;
  UserCounters counters_;
  const std::vector<Statistics>* statistics_;
  bool last_benchmark_instance_;
  int64_t repetitions_;
  double min_time_;
  IterationCount iterations_;
  int threads_;  // Number of concurrent threads to use
  // Make it mutable so it can be initialized (mutated) later on a const
  // instance.
  mutable int64_t random_interleaving_repetitions_ = -1;
};

bool FindBenchmarksInternal(const std::string& re,
                            std::vector<BenchmarkInstance>* benchmarks,
                            std::ostream* Err);

bool IsZero(double n);

ConsoleReporter::OutputOptions GetOutputOptions(bool force_no_color = false);

double GetMinTime();

int64_t GetRepetitions();

}  // end namespace internal
}  // end namespace benchmark

#endif  // BENCHMARK_API_INTERNAL_H
