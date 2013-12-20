#include "sleep.h"

#include <time.h>
#include <errno.h>

namespace benchmark {
#ifdef OS_WINDOWS
// Window's _sleep takes milliseconds argument.
void SleepForMilliseconds(int milliseconds) {
  _sleep(milliseconds);
}
void SleepForSeconds(double seconds) {
  SleepForMilliseconds(static_cast<int>(kNumMillisPerSecond * seconds));
}
#else  // OS_WINDOWS
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

