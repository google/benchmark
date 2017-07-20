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

#include "benchmark/benchmark.h"

namespace benchmark {
namespace internal {

namespace {
double Finish(Counter const& c, double cpu_time, double num_threads) {
  double v = c.value;
  if (c.flags & Counter::kIsRate) {
    v /= cpu_time;
  }
  if (c.flags & Counter::kAvgThreads) {
    v /= num_threads;
  }
  return v;
}
}  // namespace

bool UserCounters::SameNames(UserCounters const& l, UserCounters const& r) {
  if (&l == &r) return true;
  if (l.counters_.size() != r.counters_.size()) {
    return false;
  }
  for (auto const& c : l.counters_) {
    if (r.counters_.find(c.first) == r.counters_.end()) {
      return false;
    }
  }
  return true;
}

void UserCounters::Finish(double cpu_time, double num_threads) {
  for (auto &c : counters_) {
    c.second.value = internal::Finish(c.second, cpu_time, num_threads);
  }
}

void UserCounters::Increment(UserCounters const& r) {
  // add counters present in both or just in *l
  for (auto &c : counters_) {
    auto it = r.counters_.find(c.first);
    if (it != r.counters_.end()) {
      c.second.value = c.second + it->second;
    }
  }
  // add counters present in r, but not in *l
  for (auto const &tc : r.counters_) {
    auto it = counters_.find(tc.first);
    if (it == counters_.end()) {
      counters_[tc.first] = tc.second;
    }
  }
}

} // namespace internal
} // namespace benchmark
