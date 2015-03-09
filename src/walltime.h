#ifndef BENCHMARK_WALLTIME_H_
#define BENCHMARK_WALLTIME_H_

#include <string>

namespace benchmark {
typedef double WallTime;

namespace walltime {
WallTime Now();

// GIVEN: walltime, generic format string (as understood by strftime),
// a boolean flag specifying if the time is local or UTC (true=local).
// RETURNS: the formatted string. ALSO RETURNS: the remaining number of
// microseconds (never printed in the string since strftime does not understand
// it)
std::string Print(WallTime time, const char *format, bool local,
                  int *remainder_us);
}  // end namespace walltime
}  // end namespace benchmark

#endif  // BENCHMARK_WALLTIME_H_
