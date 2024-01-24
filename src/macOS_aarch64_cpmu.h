// WARNING: must be run as root on an M1 device
// WARNING: fragile, uses private apple APIs
// currently no command line interface, see variables at top of main

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

bool configure_macOS_rdtsc();

bool init_macOS_rdtsc();

unsigned long long int macOS_rdtsc();

#endif  // BENCHMARK_MACOS_AARCH64

#endif  // MACOS_AARCH64_CPMU_H_
