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

#include "benchmark/benchmark.h"
#include "commandlineflags.h"

namespace benchmark {
namespace internal {

extern const double kSafetyMultiplier;

// Information kept per benchmark we may want to run
class BenchmarkInstance {
 public:
  BenchmarkInstance(Benchmark* benchmark, const std::vector<int64_t>& args,
                    int threads);

  // Returns number of repetitions for Random Interleaving. This will be
  // initialized later once we finish the first repetition, if Random
  // Interleaving is enabled. See also ComputeRandominterleavingrepetitions().
  int RandomInterleavingRepetitions() const;

  // Returns true if repetitions for Random Interleaving is initialized.
  bool RandomInterleavingRepetitionsInitialized() const;

  // Initializes number of repetitions for random interleaving.
  void InitRandomInterleavingRepetitions(int reps) const;

  const BenchmarkName& name() const { return name_; }
  AggregationReportMode aggregation_report_mode() const {
    return aggregation_report_mode_;
  }
  TimeUnit time_unit() const { return time_unit_; }
  bool measure_process_cpu_time() const { return measure_process_cpu_time_; }
  bool use_real_time() const { return use_real_time_; }
  bool use_manual_time() const { return use_manual_time_; }
  BigO complexity() const { return complexity_; }
  BigOFunc& complexity_lambda() const { return *complexity_lambda_; }
  const std::vector<Statistics>& statistics() const { return statistics_; }
  int repetitions() const { return repetitions_; }
  double MinTime() const;
  IterationCount iterations() const { return iterations_; }
  int threads() const { return threads_; }

  bool last_benchmark_instance;

  State Run(IterationCount iters, int thread_id, internal::ThreadTimer* timer,
            internal::ThreadManager* manager,
            internal::PerfCountersMeasurement* perf_counters_measurement) const;

 private:
  BenchmarkName name_;
  Benchmark& benchmark_;
  AggregationReportMode aggregation_report_mode_;
  const std::vector<int64_t>& args_;
  TimeUnit time_unit_;
  bool measure_process_cpu_time_;
  bool use_real_time_;
  bool use_manual_time_;
  BigO complexity_;
  BigOFunc* complexity_lambda_;
  std::vector<Statistics> statistics_;
  int repetitions_;
  double min_time_;
  IterationCount iterations_;
  int threads_;
  UserCounters counters_;
  mutable int random_interleaving_repetitions_ = -1;
};

bool FindBenchmarksInternal(const std::string& re,
                            std::vector<BenchmarkInstance>* benchmarks,
                            std::ostream* Err);

bool IsZero(double n);

ConsoleReporter::OutputOptions GetOutputOptions(bool force_no_color = false);

double GetMinTime();

int GetRepetitions();

}  // end namespace internal
}  // end namespace benchmark

#endif  // BENCHMARK_API_INTERNAL_H
