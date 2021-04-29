#include "benchmark_api_internal.h"

#include <cinttypes>

#include "string_util.h"

DECLARE_bool(benchmark_enable_random_interleaving);

namespace benchmark {
namespace internal {

BenchmarkInstance::BenchmarkInstance(Benchmark* benchmark,
                                     const std::vector<int64_t>& args,
                                     int threads)
    : benchmark_(benchmark), args_(args), threads_(threads) {
  name_.function_name = benchmark->name_;

  // Add arguments to instance name
  size_t arg_i = 0;
  for (auto const& arg : args) {
    if (!name_.args.empty()) {
      name_.args += '/';
    }

    if (arg_i < benchmark->arg_names_.size()) {
      const auto& arg_name = benchmark->arg_names_[arg_i];
      if (!arg_name.empty()) {
        name_.args += StrFormat("%s:", arg_name.c_str());
      }
    }

    name_.args += StrFormat("%" PRId64, arg);
    ++arg_i;
  }

  if (!IsZero(benchmark->min_time_))
    name_.min_time = StrFormat("min_time:%0.3f", benchmark->min_time_);
  if (benchmark->iterations_ != 0) {
    name_.iterations = StrFormat(
        "iterations:%lu", static_cast<unsigned long>(benchmark->iterations_));
  }
  if (benchmark->repetitions_ != 0)
    name_.repetitions = StrFormat("repeats:%d", benchmark->repetitions_);

  if (benchmark->measure_process_cpu_time_) {
    name_.time_type = "process_time";
  }

  if (benchmark->use_manual_time_) {
    if (!name_.time_type.empty()) {
      name_.time_type += '/';
    }
    name_.time_type += "manual_time";
  } else if (benchmark->use_real_time_) {
    if (!name_.time_type.empty()) {
      name_.time_type += '/';
    }
    name_.time_type += "real_time";
  }

  // Add the number of threads used to the name
  if (!benchmark->thread_counts_.empty()) {
    name_.threads = StrFormat("threads:%d", threads_);
  }

  aggregation_report_mode_ = benchmark->aggregation_report_mode_;
  time_unit_ = benchmark->time_unit_;
  range_multiplier_ = benchmark->range_multiplier_;
  min_time_ =
      !IsZero(benchmark->min_time_) ? benchmark->min_time_ : GetMinTime();
  iterations_ = benchmark->iterations_;
  repetitions_ = benchmark->repetitions_;
  measure_process_cpu_time_ = benchmark->measure_process_cpu_time_;
  use_real_time_ = benchmark->use_real_time_;
  use_manual_time_ = benchmark->use_manual_time_;
  complexity_ = benchmark->complexity_;
  complexity_lambda_ = benchmark->complexity_lambda_;
  statistics_ = &(benchmark->statistics_);
}

const BenchmarkName& BenchmarkInstance::name() const {
  return name_;
}

AggregationReportMode BenchmarkInstance::aggregation_report_mode() const {
  return aggregation_report_mode_;
}

TimeUnit BenchmarkInstance::time_unit() const {
  return time_unit_;
}

int BenchmarkInstance::threads() const{
  return threads_;
}

bool BenchmarkInstance::measure_process_cpu_time() const {
  return measure_process_cpu_time_;
}

bool BenchmarkInstance::use_real_time() const {
  return use_real_time_;
}

bool BenchmarkInstance::use_manual_time() const {
  return use_manual_time_;
}

BigO BenchmarkInstance::complexity() const {
  return complexity_;
}

BigOFunc* BenchmarkInstance::complexity_lambda() const {
  return complexity_lambda_;
}

bool BenchmarkInstance::last_benchmark_instance() const {
  return last_benchmark_instance_;
}

IterationCount BenchmarkInstance::iterations() const {
  return iterations_;
}

int64_t BenchmarkInstance::repetitions() const {
  return repetitions_;
}

const std::vector<Statistics>* BenchmarkInstance::statistics() const {
  return statistics_;
}

double BenchmarkInstance::min_time() const {
  if (FLAGS_benchmark_enable_random_interleaving) {
    // Random Interleaving will automatically adjust
    // random_interleaving_repetitions(). Dividing
    // total execution time by random_interleaving_repetitions() gives
    // the adjusted min_time per repetition.
    return min_time_ * GetRepetitions() / random_interleaving_repetitions();
  }
  return min_time_;
}

int64_t BenchmarkInstance::random_interleaving_repetitions() const {
  return random_interleaving_repetitions_ < 0
             ? GetRepetitions()
             : random_interleaving_repetitions_;
}

bool BenchmarkInstance::random_interleaving_repetitions_initialized() const {
  return random_interleaving_repetitions_ >= 0;
}

void BenchmarkInstance::init_random_interleaving_repetitions(
    int64_t repetitions) const {
  random_interleaving_repetitions_ = repetitions;
}

State BenchmarkInstance::Run(
    IterationCount iters, int thread_id, internal::ThreadTimer* timer,
    internal::ThreadManager* manager,
    internal::PerfCountersMeasurement* perf_counters_measurement) const {
  State st(iters, args_, thread_id, threads_, timer, manager,
           perf_counters_measurement);
  benchmark_->Run(st);
  return st;
}

}  // internal
}  // benchmark
