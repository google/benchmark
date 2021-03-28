#ifndef BENCHMARK_ADJUST_REPETITIONS_H
#define BENCHMARK_ADJUST_REPETITIONS_H

#include "benchmark/benchmark.h"
#include "commandlineflags.h"

namespace benchmark {
namespace internal {

// Defines the input tuple to ComputeRandomInterleavingRepetitions().
struct InternalRandomInterleavingRepetitionsInput {
  double total_execution_time_per_repetition;
  double time_used_per_repetition;
  double real_time_used_per_repetition;
  double min_time_per_repetition;
  double max_overhead;
  size_t max_repetitions;
};

// Should be called right after the first repetition is completed to estimate
// the number of iterations.
size_t ComputeRandomInterleavingRepetitions(
    InternalRandomInterleavingRepetitionsInput input);

}  // end namespace internal
}  // end namespace benchmark

#endif  // BENCHMARK_ADJUST_REPETITIONS_H
