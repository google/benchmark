#include "benchmark_api_internal.h"

#include <cinttypes>

#include "check.h"
#include "string_util.h"

DECLARE_bool(benchmark_enable_random_interleaving);

namespace benchmark {
namespace internal {

BenchmarkInstance::BenchmarkInstance(Benchmark* benchmark,
                                     const std::vector<int64_t>& args,
                                     int thread_count)
    : benchmark_(*benchmark),
      aggregation_report_mode_(benchmark_.aggregation_report_mode_),
      args_(args),
      time_unit_(benchmark_.time_unit_),
      measure_process_cpu_time_(benchmark_.measure_process_cpu_time_),
      use_real_time_(benchmark_.use_real_time_),
      use_manual_time_(benchmark_.use_manual_time_),
      complexity_(benchmark_.complexity_),
      complexity_lambda_(benchmark_.complexity_lambda_),
      statistics_(benchmark_.statistics_),
      repetitions_(benchmark_.repetitions_),
      min_time_(!IsZero(benchmark_.min_time_) ? benchmark_.min_time_
                                              : GetMinTime()),
      iterations_(benchmark_.iterations_),
      threads_(thread_count) {
  CHECK(!IsZero(min_time_)) << "min_time must be non-zero.";

  name_.function_name = benchmark_.name_;

  size_t arg_i = 0;
  for (const auto& arg : args) {
    if (!name_.args.empty()) {
      name_.args += '/';
    }

    if (arg_i < benchmark->arg_names_.size()) {
      const auto& arg_name = benchmark_.arg_names_[arg_i];
      if (!arg_name.empty()) {
        name_.args += StrFormat("%s:", arg_name.c_str());
      }
    }

    name_.args += StrFormat("%" PRId64, arg);
    ++arg_i;
  }

  if (!IsZero(benchmark->min_time_)) {
    name_.min_time = StrFormat("min_time:%0.3f", benchmark_.min_time_);
  }

  if (benchmark_.iterations_ != 0) {
    name_.iterations = StrFormat(
        "iterations:%lu", static_cast<unsigned long>(benchmark_.iterations_));
  }

  if (benchmark_.repetitions_ != 0) {
    name_.repetitions = StrFormat("repeats:%d", benchmark_.repetitions_);
  }

  if (benchmark_.measure_process_cpu_time_) {
    name_.time_type = "process_time";
  }

  if (benchmark_.use_manual_time_) {
    if (!name_.time_type.empty()) {
      name_.time_type += '/';
    }
    name_.time_type += "manual_time";
  } else if (benchmark_.use_real_time_) {
    if (!name_.time_type.empty()) {
      name_.time_type += '/';
    }
    name_.time_type += "real_time";
  }

  if (!benchmark_.thread_counts_.empty()) {
    name_.threads = StrFormat("threads:%d", threads_);
  }
}

double BenchmarkInstance::MinTime() const {
  if (FLAGS_benchmark_enable_random_interleaving) {
    // Random Interleaving will automatically adjust
    // random_interleaving_repetitions(). Dividing
    // total execution time by random_interleaving_repetitions() gives
    // the adjusted min_time per repetition.
    return min_time_ * GetRepetitions() / RandomInterleavingRepetitions();
  }
  return min_time_;
}

int BenchmarkInstance::RandomInterleavingRepetitions() const {
  return random_interleaving_repetitions_ < 0
             ? GetRepetitions()
             : random_interleaving_repetitions_;
}

bool BenchmarkInstance::RandomInterleavingRepetitionsInitialized() const {
  return random_interleaving_repetitions_ >= 0;
}

void BenchmarkInstance::InitRandomInterleavingRepetitions(
    int reps) const {
  random_interleaving_repetitions_ = reps;
}

State BenchmarkInstance::Run(
    IterationCount iters, int thread_id, internal::ThreadTimer* timer,
    internal::ThreadManager* manager,
    internal::PerfCountersMeasurement* perf_counters_measurement) const {
  State st(iters, args_, thread_id, threads_, timer, manager,
           perf_counters_measurement);
  benchmark_.Run(st);
  return st;
}

}  // namespace internal
}  // namespace benchmark
