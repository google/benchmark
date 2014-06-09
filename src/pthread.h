#ifndef BENCHMARK_PTHREAD_H_
#define BENCHMARK_PTHREAD_H_

#include "benchmark/port.h"

#if defined HAVE_PTHREAD_H
#include <pthread.h>
#endif

#if defined OS_WINDOWS
#include "windows/pthread.h"
#endif

#endif  // BENCHMARK_PTHREAD_H_
