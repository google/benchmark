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
#include "check.h"
#include "internal_macros.h"

#include <cstring>
#include <cassert>

namespace benchmark {

double Counter::FormatValue(double cpu_time, double num_threads) const {
  switch (type) {
  case CT_Default: return value;
  case CT_Rate: return value / cpu_time;
  case CT_ThreadAverage: return value / num_threads;
  case CT_ThreadAverageRate: return (value / cpu_time) / num_threads;
  default:
    BENCHMARK_UNREACHABLE();
  }
}

std::string Counter::FormatType(TimeUnit cpu_time_unit) const {
  std::string const unit_str = GetTimeUnitString(cpu_time_unit);
  switch (type) {
  case CT_Default: return "";
  case CT_Rate: return "/" + unit_str;
  case CT_ThreadAverage: return "/thread";
  case CT_ThreadAverageRate: return "/" + unit_str + "/thread";
  default:
    BENCHMARK_UNREACHABLE();
  }
}

}  // end namespace benchmark
