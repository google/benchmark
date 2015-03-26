#ifndef BENCHMARK_WALLTIME_H_
#define BENCHMARK_WALLTIME_H_

#include <string>

namespace benchmark {
typedef double WallTime;

namespace walltime {
WallTime CPUWalltimeNow();
WallTime ChronoWalltimeNow();
WallTime Now();
}  // end namespace walltime

std::string DateTimeString(bool local = false);

inline std::string LocalDateTimeString() {
    return DateTimeString(true);
}

}  // end namespace benchmark

#endif  // BENCHMARK_WALLTIME_H_
