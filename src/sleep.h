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

#ifndef BENCHMARK_SLEEP_H_
#define BENCHMARK_SLEEP_H_

#include <stdint.h>

namespace benchmark {
const int64_t kNumMillisPerSecond = 1000LL;
const int64_t kNumMicrosPerMilli = 1000LL;
const int64_t kNumMicrosPerSecond = kNumMillisPerSecond * 1000LL;
const int64_t kNumNanosPerMicro = 1000LL;
const int64_t kNumNanosPerSecond = kNumNanosPerMicro * kNumMicrosPerSecond;

void SleepForMilliseconds(int milliseconds);
void SleepForSeconds(double seconds);
}  // end namespace benchmark

#endif  // BENCHMARK_SLEEP_H_
