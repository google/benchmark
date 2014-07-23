// Copyright 2014 Google Inc. All rights reserved.
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

#include "sleep.h"

#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "benchmark/port.h"

namespace benchmark {
#ifdef OS_WINDOWS
// Window's _sleep takes milliseconds argument.
void SleepForMilliseconds(int milliseconds) { _sleep(milliseconds); }
void SleepForSeconds(double seconds) {
  SleepForMilliseconds(static_cast<int>(kNumMillisPerSecond * seconds));
}
#else   // OS_WINDOWS
void SleepForMicroseconds(int64_t microseconds) {
  struct timespec sleep_time;
  sleep_time.tv_sec = microseconds / kNumMicrosPerSecond;
  sleep_time.tv_nsec = (microseconds % kNumMicrosPerSecond) * kNumNanosPerMicro;
  while (nanosleep(&sleep_time, &sleep_time) != 0 && errno == EINTR)
    ;  // Ignore signals and wait for the full interval to elapse.
}

void SleepForMilliseconds(int milliseconds) {
  SleepForMicroseconds(static_cast<int64_t>(milliseconds) * kNumMicrosPerMilli);
}

void SleepForSeconds(double seconds) {
  SleepForMicroseconds(static_cast<int64_t>(seconds * kNumMicrosPerSecond));
}
#endif  // OS_WINDOWS
}  // end namespace benchmark
