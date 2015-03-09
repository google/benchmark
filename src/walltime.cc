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

#include "walltime.h"

#include <sys/time.h>

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <ctime>

#include <atomic>
#include <limits>
#include <type_traits>

#include "check.h"
#include "cycleclock.h"
#include "sysinfo.h"

namespace benchmark {
namespace walltime {
namespace {

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

} // end anonymous namespace


namespace {

class WallTimeImp
{
public:
  WallTime Now();

  static WallTimeImp& GetWallTimeImp() {
    static WallTimeImp imp;
#if __cplusplus >= 201103L
    static_assert(std::is_trivially_destructible<WallTimeImp>::value,
                  "WallTimeImp must be trivially destructible to prevent "
                  "issues with static destruction");
#endif
    return imp;
  }

private:
  WallTimeImp();
  // Helper routines to load/store a float from an AtomicWord. Required because
  // g++ < 4.7 doesn't support std::atomic<float> correctly. I cannot wait to
  // get rid of this horror show.
  void SetDrift(float f) {
    int32_t w;
    memcpy(&w, &f, sizeof(f));
    std::atomic_store(&drift_adjust_, w);
  }

  float GetDrift() const {
    float f;
    int32_t w = std::atomic_load(&drift_adjust_);
    memcpy(&f, &w, sizeof(f));
    return f;
  }

  WallTime Slow() const {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
  }

private:
  static_assert(sizeof(float) <= sizeof(int32_t),
               "type sizes don't allow the drift_adjust hack");

  static constexpr double kMaxErrorInterval = 100e-6;

  WallTime base_walltime_;
  int64_t base_cycletime_;
  int64_t cycles_per_second_;
  double seconds_per_cycle_;
  uint32_t last_adjust_time_;
  std::atomic<int32_t> drift_adjust_;
  int64_t max_interval_cycles_;

  BENCHMARK_DISALLOW_COPY_AND_ASSIGN(WallTimeImp);
};


WallTime WallTimeImp::Now() {
  WallTime now = 0.0;
  WallTime result = 0.0;
  int64_t ct = 0;
  uint32_t top_bits = 0;
  do {
    ct = cycleclock::Now();
    int64_t cycle_delta = ct - base_cycletime_;
    result = base_walltime_ + cycle_delta * seconds_per_cycle_;

    top_bits = static_cast<uint32_t>(uint64_t(ct) >> 32);
    // Recompute drift no more often than every 2^32 cycles.
    // I.e., @2GHz, ~ every two seconds
    if (top_bits == last_adjust_time_) {  // don't need to recompute drift
      return result + GetDrift();
    }

    now = Slow();
  } while (cycleclock::Now() - ct > max_interval_cycles_);
  // We are now sure that "now" and "result" were produced within
  // kMaxErrorInterval of one another.

  SetDrift(now - result);
  last_adjust_time_ = top_bits;
  return now;
}


WallTimeImp::WallTimeImp()
    : base_walltime_(0.0), base_cycletime_(0),
      cycles_per_second_(0), seconds_per_cycle_(0.0),
      last_adjust_time_(0), drift_adjust_(0),
      max_interval_cycles_(0) {
  cycles_per_second_ = static_cast<int64_t>(CyclesPerSecond());
  CHECK(cycles_per_second_ != 0);
  seconds_per_cycle_ = 1.0 / cycles_per_second_;
  max_interval_cycles_ =
      static_cast<int64_t>(cycles_per_second_ * kMaxErrorInterval);
  do {
    base_cycletime_ = cycleclock::Now();
    base_walltime_ = Slow();
  } while (cycleclock::Now() - base_cycletime_ > max_interval_cycles_);
  // We are now sure that "base_walltime" and "base_cycletime" were produced
  // within kMaxErrorInterval of one another.

  SetDrift(0.0);
  last_adjust_time_ = static_cast<uint32_t>(uint64_t(base_cycletime_) >> 32);
}

} // end anonymous namespace


WallTime Now()
{
    static WallTimeImp& imp = WallTimeImp::GetWallTimeImp();
    return imp.Now();
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
