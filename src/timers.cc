// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "timers.h"
#include "internal_macros.h"

#ifdef BENCHMARK_OS_WINDOWS
#include <Shlwapi.h>
#include <VersionHelpers.h>
#include <Windows.h>
#else
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>  // this header must be included before 'sys/sysctl.h' to avoid compilation error on FreeBSD
#include <unistd.h>
#if defined BENCHMARK_OS_FREEBSD || defined BENCHMARK_OS_MACOSX
#include <sys/sysctl.h>
#endif
#if defined(BENCHMARK_OS_MACOSX)
#include <mach/mach_init.h>
#include <mach/mach_port.h>
#include <mach/thread_act.h>
#endif
#endif

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <limits>
#include <mutex>

#include "check.h"
#include "log.h"
#include "sleep.h"
#include "string_util.h"

namespace benchmark {

// Suppress unused warnings on helper functions.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

namespace {
#if defined(BENCHMARK_OS_WINDOWS)
double MakeTime(FILETIME const& kernel_time, FILETIME const& user_time) {
  ULARGE_INTEGER kernel;
  ULARGE_INTEGER user;
  kernel.HighPart = kernel_time.dwHighDateTime;
  kernel.LowPart = kernel_time.dwLowDateTime;
  user.HighPart = user_time.dwHighDateTime;
  user.LowPart = user_time.dwLowDateTime;
  return (static_cast<double>(kernel.QuadPart) +
          static_cast<double>(user.QuadPart)) *
         1e-7;
}
#else
double MakeTime(struct timespec const& ts) {
  return ts.tv_sec + (static_cast<double>(ts.tv_nsec) * 1e-9);
}
double MakeTime(struct rusage ru) {
  return (static_cast<double>(ru.ru_utime.tv_sec) +
          static_cast<double>(ru.ru_utime.tv_usec) * 1e-6 +
          static_cast<double>(ru.ru_stime.tv_sec) +
          static_cast<double>(ru.ru_stime.tv_usec) * 1e-6);
}
#endif
#if defined(BENCHMARK_OS_MACOSX)
double MakeTime(thread_basic_info_data_t const& info) {
  return (static_cast<double>(info.user_time.seconds) +
          static_cast<double>(info.user_time.microseconds) * 1e-6 +
          static_cast<double>(info.system_time.seconds) +
          static_cast<double>(info.user_time.microseconds) * 1e-6);
}
#endif

}  // end namespace

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

double ProcessCPUUsage() {
#if defined(CLOCK_PROCESS_CPUTIME_ID)
  struct timespec spec;
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &spec) == 0) {
    return MakeTime(spec);
  }
  return 0.0;
#elif defined(BENCHMARK_OS_WINDOWS)
  HANDLE proc = GetCurrentProcess();
  FILETIME creation_time;
  FILETIME exit_time;
  FILETIME kernel_time;
  FILETIME user_time;
  GetProcessTimes(proc, &creation_time, &exit_time, &kernel_time, &user_time);
  return MakeTime(kernel_time, user_time);
#else
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) {
    return MakeTime(ru);
  }
  return 0.0;
#endif
}

double ChildrenCPUUsage() {
#ifndef BENCHMARK_OS_WINDOWS
  struct rusage ru;
  if (getrusage(RUSAGE_CHILDREN, &ru) == 0) {
    return MakeTime(ru);
  } else {
    return 0.0;
  }
#else
  // TODO: Not sure what this even means on Windows
  return 0.0;
#endif  // BENCHMARK_OS_WINDOWS
}

double ThreadCPUUsage() {
#if defined(CLOCK_THREAD_CPUTIME_ID)
  struct timespec ts;
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) {
    std::cerr << "Failed to get thread CPU time using clock_gettime"
              << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return ts.tv_sec + (static_cast<double>(ts.tv_nsec) * 1e-9);
#elif defined(BENCHMARK_OS_WINDOWS)
  HANDLE this_thread = GetCurrentThread();
  FILETIME creation_time;
  FILETIME exit_time;
  FILETIME kernel_time;
  FILETIME user_time;
  GetThreadTimes(this_thread, &creation_time, &exit_time, &kernel_time,
                 &user_time);
  return MakeTime(kernel_time, user_time);
#elif defined(BENCHMARK_OS_MACOSX)
  mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
  thread_basic_info_data_t info;
  mach_port_t thread = mach_thread_self();
  kern_return_t kr = thread_info(thread, THREAD_BASIC_INFO, (thread_info_t) &info, &count);
  if (kr == KERN_SUCCESS && (info.flags & TH_FLAGS_IDLE) == 0) {
    double t = MakeTime(info);
    mach_port_deallocate(mach_task_self(), thread);
    return t;
  } else {
      std::cerr << "Could not retrieve thread info" << std::endl;
      std::exit(EXIT_FAILURE);
  }
#else
#error Per-thread timing is not available on your system.
#endif
}

namespace {

std::string DateTimeString(bool local) {
  typedef std::chrono::system_clock Clock;
  std::time_t now = Clock::to_time_t(Clock::now());
  const std::size_t kStorageSize = 128;
  char storage[kStorageSize];
  std::size_t written;

  if (local) {
#if defined(BENCHMARK_OS_WINDOWS)
    written =
        std::strftime(storage, sizeof(storage), "%x %X", ::localtime(&now));
#else
    std::tm timeinfo;
    std::memset(&timeinfo, 0, sizeof(std::tm));
    ::localtime_r(&now, &timeinfo);
    written = std::strftime(storage, sizeof(storage), "%F %T", &timeinfo);
#endif
  } else {
#if defined(BENCHMARK_OS_WINDOWS)
    written = std::strftime(storage, sizeof(storage), "%x %X", ::gmtime(&now));
#else
    std::tm timeinfo;
    std::memset(&timeinfo, 0, sizeof(std::tm));
    ::gmtime_r(&now, &timeinfo);
    written = std::strftime(storage, sizeof(storage), "%F %T", &timeinfo);
#endif
  }
  CHECK(written < kStorageSize);
  ((void)written);  // prevent unused variable in optimized mode.
  return std::string(storage);
}

}  // end namespace

std::string LocalDateTimeString() { return DateTimeString(true); }

}  // end namespace benchmark
