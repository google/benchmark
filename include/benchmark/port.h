/* Copyright (c) 2008, Google Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ---
 * Author: Craig Silverstein
 *
 * These are some portability typedefs and defines to make it a bit
 * easier to compile this code under VC++.
 *
 * Several of these are taken from glib:
 *    http://developer.gnome.org/doc/API/glib/glib-windows-compatability-functions.html
 */

#ifndef BENCHMARK_PORT_H_
#define BENCHMARK_PORT_H_

#ifdef _WIN32

#define NOMINMAX

#ifdef _MSC_VER
#define COMPILER_MSVC
#endif

#include <windows.h>
#include <winsock.h>         /* for timeval */
#include <stdio.h>           /* read in vsnprintf decl. before redifining it */
#include <time.h>            /* for localtime_s() */
#include <Shlwapi.h>

/* 4244: otherwise we get problems when substracting two size_t's to an int
 * 4996: Yes, we're ok using "unsafe" functions like fopen() and strerror()
 */
#pragma warning(disable:4244 4996)

/* We can't just use _vsnprintf and _snprintf as drop-in-replacements,
 * because they don't always NUL-terminate. :-(  We also can't use the
 * name vsnprintf, since windows defines that (but not snprintf (!)).
 */
extern int snprintf(char *str, size_t size,
                                       const char *format, ...);

inline struct tm* localtime_r(const time_t* timep, struct tm* result) {
  localtime_s(result, timep);
  return result;
}

inline struct tm* gmtime_r(const time_t *timer, struct tm *result) {
  errno_t e = gmtime_s(result, timer);
  return result;
}

extern int gettimeofday(struct timeval *tv, void* tz);

// The rest of this file is a poor man's config.h.

#undef  HAVE_GNUREGEX_H
#undef  HAVE_PTHREAD_H
#define HAVE_REGEX
#undef  HAVE_REGEX_H
#undef  HAVE_SEMAPHORE_H
#undef  HAVE_SYS_RESOURCE_H
#undef  HAVE_SYS_SYSCTL_H
#undef  HAVE_SYS_TIME_H
#undef  HAVE_UNISTD_H

#else  /* _WIN32 */

#undef  HAVE_GNUREGEX_H
#define HAVE_PTHREAD_H
#undef  HAVE_REGEX
#define HAVE_REGEX_H
#define HAVE_SEMAPHORE_H
#define HAVE_SYS_RESOURCE_H
#define HAVE_SYS_SYSCTL_H
#define HAVE_SYS_TIME_H
#define HAVE_UNISTD_H

#endif  /* _WIN32 */

#endif  /* BENCHMARK_PORT_H_ */
