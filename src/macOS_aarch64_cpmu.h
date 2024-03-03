// WARNING: must be run as root on an M1 device
// WARNING: fragile, uses private apple APIs

/*
  Based on https://github.com/travisdowns/robsize
  Henry Wong <henry@stuffedcow.net>
  http://blog.stuffedcow.net/2013/05/measuring-rob-capacity/
  2014-10-14
*/

#ifndef MACOS_AARCH64_CPMU_H_
#define MACOS_AARCH64_CPMU_H_

#include "internal_macros.h"

#ifdef BENCHMARK_MACOS_AARCH64

namespace benchmark {
namespace cycleclock {

/*
  Initialise and configure the rdtsc counter modules.
  Return false if it failed to initialise or to configure,
  otherwise return true.
 */
bool init_macOS_rdtsc();

/*
  Configure the counter, such as clock cycles.
  Return false if it failed to configure, otherwise return true.
 */
bool configure_macOS_rdtsc();

/*
  Return the counter value. It returns 0 in case function failed
  to get the value.
 */
unsigned long long int macOS_rdtsc();

}  // namespace cycleclock
}  // namespace benchmark

#endif  // BENCHMARK_MACOS_AARCH64

#endif  // MACOS_AARCH64_CPMU_H_
