#ifndef BENCHMARK_WALLTIME_H_
#define BENCHMARK_WALLTIME_H_

#include "internal_macros.h"

#include <chrono>
#include <string>

namespace benchmark {
typedef double WallTime;

namespace walltime {

#if defined(HAVE_STEADY_CLOCK)
template <bool HighResIsSteady = std::chrono::high_resolution_clock::is_steady>
struct ChooseSteadyClock {
    typedef std::chrono::high_resolution_clock type;
};

template <>
struct ChooseSteadyClock<false> {
    typedef std::chrono::steady_clock type;
};
#endif

struct ChooseClockType {
#if defined(HAVE_STEADY_CLOCK)
  typedef ChooseSteadyClock<>::type type;
#else
  typedef std::chrono::high_resolution_clock type;
#endif
};

inline double ChronoClockNow() {
    typedef ChooseClockType::type ClockType;
    using FpSeconds =
        std::chrono::duration<double, std::chrono::seconds::period>;
    return FpSeconds(ClockType::now().time_since_epoch()).count();
}

WallTime Now();
}  // end namespace walltime

std::string LocalDateTimeString();

}  // end namespace benchmark

#endif  // BENCHMARK_WALLTIME_H_
