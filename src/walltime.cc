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

#include "walltime.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <atomic>
#include <limits>

#include "benchmark/macros.h"
#include "cycleclock.h"
#include "sysinfo.h"

namespace benchmark {
namespace walltime {
namespace {
const double kMaxErrorInterval = 100e-6;

std::atomic<bool> initialized(false);
WallTime base_walltime = 0.0;
int64_t base_cycletime = 0;
int64_t cycles_per_second;
double seconds_per_cycle;
uint32_t last_adjust_time = 0;
std::atomic<int32_t> drift_adjust(0);
int64_t max_interval_cycles = 0;

// Helper routines to load/store a float from an AtomicWord. Required because
// g++ < 4.7 doesn't support std::atomic<float> correctly. I cannot wait to get
// rid of this horror show.
inline void SetDrift(float f) {
  int32_t w;
  memcpy(&w, &f, sizeof(f));
  std::atomic_store(&drift_adjust, w);
}

inline float GetDrift() {
  float f;
  int32_t w = std::atomic_load(&drift_adjust);
  memcpy(&f, &w, sizeof(f));
  return f;
}

static_assert(sizeof(float) <= sizeof(int32_t),
              "type sizes don't allow the drift_adjust hack");

WallTime Slow() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec * 1e-6;
}

bool SplitTimezone(WallTime value, bool local, struct tm* t,
                   double* subsecond) {
  memset(t, 0, sizeof(*t));
  if ((value < 0) || (value > std::numeric_limits<time_t>::max())) {
    *subsecond = 0.0;
    return false;
  }
  const time_t whole_time = static_cast<time_t>(value);
  *subsecond = value - whole_time;
  if (local)
    localtime_r(&whole_time, t);
  else
    gmtime_r(&whole_time, t);
  return true;
}
}  // end namespace

// This routine should be invoked to initialize walltime.
// It is not intended for general purpose use.
void Initialize() {
  CHECK(!std::atomic_load(&initialized));
  cycles_per_second = static_cast<int64_t>(CyclesPerSecond());
  CHECK(cycles_per_second != 0);
  seconds_per_cycle = 1.0 / cycles_per_second;
  max_interval_cycles =
      static_cast<int64_t>(cycles_per_second * kMaxErrorInterval);
  do {
    base_cycletime = cycleclock::Now();
    base_walltime = Slow();
  } while (cycleclock::Now() - base_cycletime > max_interval_cycles);
  // We are now sure that "base_walltime" and "base_cycletime" were produced
  // within kMaxErrorInterval of one another.

  SetDrift(0.0);
  last_adjust_time = static_cast<uint32_t>(uint64_t(base_cycletime) >> 32);
  std::atomic_store(&initialized, true);
}

WallTime Now() {
  if (!std::atomic_load(&initialized)) return Slow();

  WallTime now = 0.0;
  WallTime result = 0.0;
  int64_t ct = 0;
  uint32_t top_bits = 0;
  do {
    ct = cycleclock::Now();
    int64_t cycle_delta = ct - base_cycletime;
    result = base_walltime + cycle_delta * seconds_per_cycle;

    top_bits = static_cast<uint32_t>(uint64_t(ct) >> 32);
    // Recompute drift no more often than every 2^32 cycles.
    // I.e., @2GHz, ~ every two seconds
    if (top_bits == last_adjust_time) {  // don't need to recompute drift
      return result + GetDrift();
    }

    now = Slow();
  } while (cycleclock::Now() - ct > max_interval_cycles);
  // We are now sure that "now" and "result" were produced within
  // kMaxErrorInterval of one another.

  SetDrift(now - result);
  last_adjust_time = top_bits;
  return now;
}

std::string Print(WallTime time, const char* format, bool local,
                  int* remainder_us) {
  char storage[32];
  struct tm split;
  double subsecond;
  if (!SplitTimezone(time, local, &split, &subsecond)) {
    snprintf(storage, sizeof(storage), "Invalid time: %f", time);
  } else {
    if (remainder_us != NULL) {
      *remainder_us = static_cast<int>((subsecond * 1000000) + 0.5);
      if (*remainder_us > 999999) *remainder_us = 999999;
      if (*remainder_us < 0) *remainder_us = 0;
    }
    strftime(storage, sizeof(storage), format, &split);
  }
  return std::string(storage);
}
}  // end namespace walltime
}  // end namespace benchmark
