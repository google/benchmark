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
  int max_repetitions;
};

// Should be called right after the first repetition is completed to estimate
// the number of iterations.
int ComputeRandomInterleavingRepetitions(
    InternalRandomInterleavingRepetitionsInput input);

}  // end namespace internal
}  // end namespace benchmark

#endif  // BENCHMARK_ADJUST_REPETITIONS_H
