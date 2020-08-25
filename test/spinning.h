#ifndef TEST_SPINNING_H
#define TEST_SPINNING_H

#include <chrono>
#include "../src/timers.h"

static const std::chrono::duration<double, std::milli> time_frame(50);
static const double time_frame_in_sec(
    std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(
        time_frame)
        .count());

void TimedBusySpinwait(const std::chrono::duration<double, std::milli>& tf) {
  const auto start = benchmark::ChronoClockNow();

  while (true) {
    const auto now = benchmark::ChronoClockNow();
    const auto elapsed = now - start;

    if (std::chrono::duration<double, std::chrono::seconds::period>(elapsed) >= tf)
      return;
  }
}

inline void MyBusySpinwait() {
  TimedBusySpinwait(time_frame);
}

#endif  // TEST_SPINNING_H