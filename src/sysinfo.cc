// Copyright 2014 Google Inc. All rights reserved.
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

#include "sysinfo.h"

#include "benchmark/port.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if defined HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#if defined HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/types.h>
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <iostream>
#include <limits>

#include "benchmark/macros.h"
#include "cycleclock.h"
#include "mutex_lock.h"
#include "pthread.h"
#include "sleep.h"

namespace benchmark {
namespace {
const int64_t estimate_time_ms = 1000;
pthread_once_t cpuinfo_init = PTHREAD_ONCE_INIT;
double cpuinfo_cycles_per_second = 1.0;
int cpuinfo_num_cpus = 1;  // Conservative guess
pthread_mutex_t cputimens_mutex;

#if !defined OS_MACOSX
// Helper function estimates cycles/sec by observing cycles elapsed during
// sleep(). Using small sleep time decreases accuracy significantly.
int64_t EstimateCyclesPerSecond() {
  const int64_t start_ticks = cycleclock::Now();
  SleepForMilliseconds(estimate_time_ms);
  return cycleclock::Now() - start_ticks;
}
#endif

#if defined OS_LINUX || defined OS_CYGWIN
// Helper function for reading an int from a file. Returns true if successful
// and the memory location pointed to by value is set to the value read.
bool ReadIntFromFile(const char* file, int* value) {
  bool ret = false;
  int fd = open(file, O_RDONLY);
  if (fd != -1) {
    char line[1024];
    char* err;
    memset(line, '\0', sizeof(line));
    CHECK(read(fd, line, sizeof(line) - 1));
    const int temp_value = strtol(line, &err, 10);
    if (line[0] != '\0' && (*err == '\n' || *err == '\0')) {
      *value = temp_value;
      ret = true;
    }
    close(fd);
  }
  return ret;
}
#endif

void InitializeSystemInfo() {
  // TODO: destroy this
  pthread_mutex_init(&cputimens_mutex, NULL);

#if defined OS_LINUX || defined OS_CYGWIN
  char line[1024];
  char* err;
  int freq;

  bool saw_mhz = false;

  // If the kernel is exporting the tsc frequency use that. There are issues
  // where cpuinfo_max_freq cannot be relied on because the BIOS may be
  // exporintg an invalid p-state (on x86) or p-states may be used to put the
  // processor in a new mode (turbo mode). Essentially, those frequencies
  // cannot always be relied upon. The same reasons apply to /proc/cpuinfo as
  // well.
  if (!saw_mhz &&
      ReadIntFromFile("/sys/devices/system/cpu/cpu0/tsc_freq_khz", &freq)) {
    // The value is in kHz (as the file name suggests).  For example, on a
    // 2GHz warpstation, the file contains the value "2000000".
    cpuinfo_cycles_per_second = freq * 1000.0;
    saw_mhz = true;
  }

  // If CPU scaling is in effect, we want to use the *maximum* frequency,
  // not whatever CPU speed some random processor happens to be using now.
  if (!saw_mhz &&
      ReadIntFromFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq",
                      &freq)) {
    // The value is in kHz.  For example, on a 2GHz warpstation, the file
    // contains the value "2000000".
    cpuinfo_cycles_per_second = freq * 1000.0;
    saw_mhz = true;
  }

  // Read /proc/cpuinfo for other values, and if there is no cpuinfo_max_freq.
  const char* pname = "/proc/cpuinfo";
  int fd = open(pname, O_RDONLY);
  if (fd == -1) {
    perror(pname);
    if (!saw_mhz) {
      cpuinfo_cycles_per_second = EstimateCyclesPerSecond();
    }
    return;
  }

  double bogo_clock = 1.0;
  bool saw_bogo = false;
  int max_cpu_id = 0;
  int num_cpus = 0;
  line[0] = line[1] = '\0';
  int chars_read = 0;
  do {  // we'll exit when the last read didn't read anything
    // Move the next line to the beginning of the buffer
    const int oldlinelen = strlen(line);
    if (sizeof(line) == oldlinelen + 1)  // oldlinelen took up entire line
      line[0] = '\0';
    else  // still other lines left to save
      memmove(line, line + oldlinelen + 1, sizeof(line) - (oldlinelen + 1));
    // Terminate the new line, reading more if we can't find the newline
    char* newline = strchr(line, '\n');
    if (newline == NULL) {
      const int linelen = strlen(line);
      const int bytes_to_read = sizeof(line) - 1 - linelen;
      CHECK(bytes_to_read > 0);  // because the memmove recovered >=1 bytes
      chars_read = read(fd, line + linelen, bytes_to_read);
      line[linelen + chars_read] = '\0';
      newline = strchr(line, '\n');
    }
    if (newline != NULL) *newline = '\0';

    // When parsing the "cpu MHz" and "bogomips" (fallback) entries, we only
    // accept postive values. Some environments (virtual machines) report zero,
    // which would cause infinite looping in WallTime_Init.
    if (!saw_mhz && strncasecmp(line, "cpu MHz", sizeof("cpu MHz") - 1) == 0) {
      const char* freqstr = strchr(line, ':');
      if (freqstr) {
        cpuinfo_cycles_per_second = strtod(freqstr + 1, &err) * 1000000.0;
        if (freqstr[1] != '\0' && *err == '\0' && cpuinfo_cycles_per_second > 0)
          saw_mhz = true;
      }
    } else if (strncasecmp(line, "bogomips", sizeof("bogomips") - 1) == 0) {
      const char* freqstr = strchr(line, ':');
      if (freqstr) {
        bogo_clock = strtod(freqstr + 1, &err) * 1000000.0;
        if (freqstr[1] != '\0' && *err == '\0' && bogo_clock > 0)
          saw_bogo = true;
      }
    } else if (strncasecmp(line, "processor", sizeof("processor") - 1) == 0) {
      num_cpus++;  // count up every time we see an "processor :" entry
      const char* freqstr = strchr(line, ':');
      if (freqstr) {
        const int cpu_id = strtol(freqstr + 1, &err, 10);
        if (freqstr[1] != '\0' && *err == '\0' && max_cpu_id < cpu_id)
          max_cpu_id = cpu_id;
      }
    }
  } while (chars_read > 0);
  close(fd);

  if (!saw_mhz) {
    if (saw_bogo) {
      // If we didn't find anything better, we'll use bogomips, but
      // we're not happy about it.
      cpuinfo_cycles_per_second = bogo_clock;
    } else {
      // If we don't even have bogomips, we'll use the slow estimation.
      cpuinfo_cycles_per_second = EstimateCyclesPerSecond();
    }
  }
  if (num_cpus == 0) {
    fprintf(stderr, "Failed to read num. CPUs correctly from /proc/cpuinfo\n");
  } else {
    if ((max_cpu_id + 1) != num_cpus) {
      fprintf(stderr,
              "CPU ID assignments in /proc/cpuinfo seems messed up."
              " This is usually caused by a bad BIOS.\n");
    }
    cpuinfo_num_cpus = num_cpus;
  }

#elif defined OS_FREEBSD
// For this sysctl to work, the machine must be configured without
// SMP, APIC, or APM support.  hz should be 64-bit in freebsd 7.0
// and later.  Before that, it's a 32-bit quantity (and gives the
// wrong answer on machines faster than 2^32 Hz).  See
//  http://lists.freebsd.org/pipermail/freebsd-i386/2004-November/001846.html
// But also compare FreeBSD 7.0:
//  http://fxr.watson.org/fxr/source/i386/i386/tsc.c?v=RELENG70#L223
//  231         error = sysctl_handle_quad(oidp, &freq, 0, req);
// To FreeBSD 6.3 (it's the same in 6-STABLE):
//  http://fxr.watson.org/fxr/source/i386/i386/tsc.c?v=RELENG6#L131
//  139         error = sysctl_handle_int(oidp, &freq, sizeof(freq), req);
#if __FreeBSD__ >= 7
  uint64_t hz = 0;
#else
  unsigned int hz = 0;
#endif
  size_t sz = sizeof(hz);
  const char* sysctl_path = "machdep.tsc_freq";
  if (sysctlbyname(sysctl_path, &hz, &sz, NULL, 0) != 0) {
    fprintf(stderr, "Unable to determine clock rate from sysctl: %s: %s\n",
            sysctl_path, strerror(errno));
    cpuinfo_cycles_per_second = EstimateCyclesPerSecond();
  } else {
    cpuinfo_cycles_per_second = hz;
  }
// TODO: also figure out cpuinfo_num_cpus

#elif defined OS_WINDOWS
#pragma comment(lib, "shlwapi.lib")  // for SHGetValue()
  // In NT, read MHz from the registry. If we fail to do so or we're in win9x
  // then make a crude estimate.
  OSVERSIONINFO os;
  os.dwOSVersionInfoSize = sizeof(os);
  DWORD data, data_size = sizeof(data);
  if (GetVersionEx(&os) && os.dwPlatformId == VER_PLATFORM_WIN32_NT &&
      SUCCEEDED(
          SHGetValueA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      "~MHz", NULL, &data, &data_size)))
    cpuinfo_cycles_per_second = (int64_t)data * (int64_t)(1000 * 1000);  // was mhz
  else
    cpuinfo_cycles_per_second = EstimateCyclesPerSecond();
// TODO: also figure out cpuinfo_num_cpus

#elif defined OS_MACOSX
  // returning "mach time units" per second. the current number of elapsed
  // mach time units can be found by calling uint64 mach_absolute_time();
  // while not as precise as actual CPU cycles, it is accurate in the face
  // of CPU frequency scaling and multi-cpu/core machines.
  // Our mac users have these types of machines, and accuracy
  // (i.e. correctness) trumps precision.
  // See cycleclock.h: CycleClock::Now(), which returns number of mach time
  // units on Mac OS X.
  mach_timebase_info_data_t timebase_info;
  mach_timebase_info(&timebase_info);
  double mach_time_units_per_nanosecond =
      static_cast<double>(timebase_info.denom) /
      static_cast<double>(timebase_info.numer);
  cpuinfo_cycles_per_second = mach_time_units_per_nanosecond * 1e9;

  int num_cpus = 0;
  size_t size = sizeof(num_cpus);
  int numcpus_name[] = {CTL_HW, HW_NCPU};
  if (::sysctl(numcpus_name, arraysize(numcpus_name), &num_cpus, &size, 0, 0) ==
          0 &&
      (size == sizeof(num_cpus)))
    cpuinfo_num_cpus = num_cpus;

#else
  // Generic cycles per second counter
  cpuinfo_cycles_per_second = EstimateCyclesPerSecond();
#endif
}
}  // end namespace

#ifndef OS_WINDOWS
// getrusage() based implementation of MyCPUUsage
static double MyCPUUsageRUsage() {
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) {
    return (static_cast<double>(ru.ru_utime.tv_sec) +
            static_cast<double>(ru.ru_utime.tv_usec) * 1e-6 +
            static_cast<double>(ru.ru_stime.tv_sec) +
            static_cast<double>(ru.ru_stime.tv_usec) * 1e-6);
  } else {
    return 0.0;
  }
}

static bool MyCPUUsageCPUTimeNsLocked(double* cputime) {
  static int cputime_fd = -1;
  if (cputime_fd == -1) {
    cputime_fd = open("/proc/self/cputime_ns", O_RDONLY);
    if (cputime_fd < 0) {
      cputime_fd = -1;
      return false;
    }
  }
  char buff[64];
  memset(buff, 0, sizeof(buff));
  if (pread(cputime_fd, buff, sizeof(buff) - 1, 0) <= 0) {
    close(cputime_fd);
    cputime_fd = -1;
    return false;
  }
  unsigned long long result = strtoull(buff, NULL, 0);
  if (result == (std::numeric_limits<unsigned long long>::max)()) {
    close(cputime_fd);
    cputime_fd = -1;
    return false;
  }
  *cputime = static_cast<double>(result) / 1e9;
  return true;
}

double MyCPUUsage() {
  {
    mutex_lock l(&cputimens_mutex);
    static bool use_cputime_ns = true;
    if (use_cputime_ns) {
      double value;
      if (MyCPUUsageCPUTimeNsLocked(&value)) {
        return value;
      }
      // Once MyCPUUsageCPUTimeNsLocked fails once fall back to getrusage().
      std::cout << "Reading /proc/self/cputime_ns failed. Using getrusage().\n";
      use_cputime_ns = false;
    }
  }
  return MyCPUUsageRUsage();
}

double ChildrenCPUUsage() {
  struct rusage ru;
  if (getrusage(RUSAGE_CHILDREN, &ru) == 0) {
    return (static_cast<double>(ru.ru_utime.tv_sec) +
            static_cast<double>(ru.ru_utime.tv_usec) * 1e-6 +
            static_cast<double>(ru.ru_stime.tv_sec) +
            static_cast<double>(ru.ru_stime.tv_usec) * 1e-6);
  } else {
    return 0.0;
  }
}
#else
double MyCPUUsage() {
  HANDLE h_process = GetCurrentProcess();
  FILETIME ft_creation;
  FILETIME ft_exit;
  FILETIME ft_kernel;
  FILETIME ft_user;
  ULARGE_INTEGER kernel;
  ULARGE_INTEGER user;

  GetProcessTimes(h_process, &ft_creation, &ft_exit, &ft_kernel, &ft_user);
  kernel.HighPart = ft_kernel.dwHighDateTime;
  kernel.LowPart = ft_kernel.dwLowDateTime;
  user.HighPart = ft_user.dwHighDateTime;
  user.LowPart = ft_user.dwLowDateTime;
  return ((double)kernel.QuadPart + (double)user.QuadPart) * 1.0E-7;
}

double ChildrenCPUUsage() {
  // TODO(pleroy): Figure out what to do here.  It's not even clear what meaning
  // it has on Windows.
  return 0;
}
#endif  // OS_WINDOWS

double CyclesPerSecond(void) {
  pthread_once(&cpuinfo_init, &InitializeSystemInfo);
  return cpuinfo_cycles_per_second;
}

int NumCPUs(void) {
  pthread_once(&cpuinfo_init, &InitializeSystemInfo);
  return cpuinfo_num_cpus;
}
}  // end namespace benchmark
