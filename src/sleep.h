#ifndef BENCHMARK_SLEEP_H_
#define BENCHMARK_SLEEP_H_

#include <stdint.h>

namespace benchmark {
void SleepForMicroseconds(int64_t microseconds);
void SleepForMilliseconds(int milliseconds);
void SleepForSeconds(double seconds);
}  // end namespace benchmark

#endif  // BENCHMARK_SLEEP_H_
