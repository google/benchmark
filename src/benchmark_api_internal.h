#ifndef BENCHMARK_API_INTERNAL_H
#define BENCHMARK_API_INTERNAL_H

#include "benchmark/benchmark.h"

#include <cmath>
#include <iosfwd>
#include <limits>
#include <string>
#include <vector>

namespace benchmark {
namespace internal {

// Information kept per benchmark we may want to run
struct Benchmark::Instance {
  std::string name;
  Benchmark* benchmark;
  ReportMode report_mode;
  std::vector<int64_t> arg;
  TimeUnit time_unit;
  int64_t range_multiplier;
  bool use_real_time;
  bool use_manual_time;
  BigO complexity;
  BigOFunc* complexity_lambda;
  UserCounters counters;
  const std::vector<Statistics>* statistics;
  bool last_benchmark_instance;
  int64_t repetitions;
  double min_time;
  size_t iterations;
  int64_t threads;  // Number of concurrent threads to us
};

bool FindBenchmarksInternal(const std::string& re,
                            std::vector<Benchmark::Instance>* benchmarks,
                            std::ostream* Err);

bool IsZero(double n);

ConsoleReporter::OutputOptions GetOutputOptions(bool force_no_color = false);

}  // end namespace internal
}  // end namespace benchmark

#endif  // BENCHMARK_API_INTERNAL_H
