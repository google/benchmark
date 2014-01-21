#ifndef BENCHMARK_SLEEP_H_
#define BENCHMARK_SLEEP_H_

#include <stdint.h>

namespace benchmark {
const int64_t kNumMillisPerSecond = 1000LL;
const int64_t kNumMicrosPerMilli = 1000LL;
const int64_t kNumMicrosPerSecond = kNumMillisPerSecond * 1000LL;
const int64_t kNumNanosPerMicro = 1000LL;
const int64_t kNumNanosPerSecond = kNumNanosPerMicro * kNumMicrosPerSecond;

void SleepForMicroseconds(int64_t microseconds);
void SleepForMilliseconds(int milliseconds);
void SleepForSeconds(double seconds);
}  // end namespace benchmark

#endif  // BENCHMARK_SLEEP_H_
