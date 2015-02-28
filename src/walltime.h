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

#ifndef BENCHMARK_WALLTIME_H_
#define BENCHMARK_WALLTIME_H_

#include <string>

namespace benchmark {
typedef double WallTime;

namespace walltime {
void Initialize();
WallTime Now();

// GIVEN: walltime, generic format string (as understood by strftime),
// a boolean flag specifying if the time is local or UTC (true=local).
// RETURNS: the formatted string. ALSO RETURNS: the remaining number of
// microseconds (never printed in the string since strftime does not understand
// it)
std::string Print(WallTime time, const char *format, bool local,
                  int *remainder_us);
}  // end namespace walltime
}  // end namespace benchmark

#endif  // BENCHMARK_WALLTIME_H_
