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

#include "benchmark_adjust_repetitions.h"

#include "benchmark_api_internal.h"
#include "log.h"

namespace benchmark {
namespace internal {

namespace {

constexpr double kNanosecondInSecond = 1e-9;

}  // namespace

int ComputeRandomInterleavingRepetitions(
    InternalRandomInterleavingRepetitionsInput input) {
  // Find the repetitions such that total overhead is bounded. Let
  //   n = desired number of repetitions, i.e., the output of this method.
  //   t = total real execution time per repetition including overhead,
  //       (input.total_execution_time_per_repetition).
  //   o = maximum allowed increase in total real execution time due to random
  //       interleaving, measured as a fraction (input.max_overhead).
  //   e = estimated total execution time without Random Interleaving
  // We want
  //   t * n / e <= 1 + o
  // I.e.,
  //   n <= (1 + o) * e / t
  //
  // Let
  //   h = overhead per repetition, which include all setup / teardown time and
  //       also the execution time of preliminary trials used to search for the
  //       correct number of iterations.
  //   r = real execution time per repetition not including overhead
  //       (input.real_accumulated_time_per_repetition).
  //   s = measured execution time per repetition not including overhead,
  //       which can be either real or CPU time
  //       (input.accumulated_time_per_repetition).
  // We have
  //   h = t - r
  //
  // Let
  //   m = total minimum measured execution time for all repetitions
  //       (input.min_time_per_repetition * input.max_repetitions).
  // Let
  //   f = m / s
  // f is the scale factor between m and s, and will be used to estimate
  // l, the total real execution time for all repetitions excluding the
  // overhead. It's reasonable to assume that the real execution time excluding
  // the overhead is proportional to the measured time. Hence we expect to see
  // l / r to be equal to m / s. That is, l / r = f, thus, l = r * f. Then the
  // total execution time e can be estimated by h + l, which is h + r * f.
  //   e = h + r * f
  // Note that this might be an underestimation. If number of repetitions is
  // reduced, we may need to run more iterations per repetition, and that may
  // increase the number of preliminary trials needed to find the correct
  // number of iterations.

  double h = std::max(0.0, input.total_execution_time_per_repetition -
                               input.real_time_used_per_repetition);
  double r =
      std::max(input.real_time_used_per_repetition, kNanosecondInSecond);
  double s =
      std::max(input.time_used_per_repetition, kNanosecondInSecond);
  double m = input.min_time_per_repetition * input.max_repetitions;

  // f = m / s
  // RunBenchmark() always overshoot the iteration count by kSafetyMultiplier.
  // Apply the same factor here.
  //   f = kSafetyMultiplier * m / s
  // Also we want to make sure 1 <= f <= input.max_repetitions. Note that we
  // may not be able to reach m because the total iters per repetition is
  // upper bounded by --benchmark_max_iters. This behavior is preserved in
  // Random Interleaving, as we won't run repetitions more than
  // input.max_repetitions to reach m.

  double f = kSafetyMultiplier * m / s;
  f = std::min(std::max(f, 1.0), static_cast<double>(input.max_repetitions));

  double e = h + r * f;
  // n <= (1 + o) * e / t = (1 + o) * e / (h + r)
  // Also we want to make sure 1 <= n <= input.max_repetition, and (h + r) > 0.
  double n = (1 + input.max_overhead) * e / (h + r);
  n = std::min(std::max(n, 1.0), static_cast<double>(input.max_repetitions));

  int n_int = static_cast<int>(n);

  VLOG(2) << "Computed random interleaving repetitions"
          << "\n  input.total_execution_time_per_repetition: "
          << input.total_execution_time_per_repetition
          << "\n  input.time_used_per_repetition: "
          << input.time_used_per_repetition
          << "\n  input.real_time_used_per_repetition: "
          << input.real_time_used_per_repetition
          << "\n  input.min_time_per_repetitions: "
          << input.min_time_per_repetition
          << "\n  input.max_repetitions: " << input.max_repetitions
          << "\n  input.max_overhead: " << input.max_overhead
          << "\n  h: " << h
          << "\n  r: " << r
          << "\n  s: " << s
          << "\n  f: " << f
          << "\n  m: " << m
          << "\n  e: " << e
          << "\n  n: " << n
          << "\n  n_int: " << n_int;

  return n_int;
}

}  // internal
}  // benchmark
