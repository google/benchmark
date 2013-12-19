#ifndef BENCHMARK_WALLTIME_H_
#define BENCHMARK_WALLTIME_H_

typedef double WallTime;

namespace walltime {
void Initialize();
WallTime Now();

// GIVEN: walltime, generic format string (as understood by strftime),
// a boolean flag specifying if the time is local or UTC (true=local).
// RETURNS: the formatted string. ALSO RETURNS: the storage printbuffer
// passed and the remaining number of microseconds (never printed in
// the string since strftime does not understand it)
const char* Print(WallTime time, const char *format, bool local,
                  char* storage, int *remainder_us);
}  // end namespace walltime

#endif  // BENCHMARK_WALLTIME_H_
