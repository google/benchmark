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
#ifndef BENCHMARK_MACROS_H_
#define BENCHMARK_MACROS_H_

#ifndef __has_feature
# define __has_feature(x) 0
#endif

#if __cplusplus < 201103L
# define DISALLOW_COPY_AND_ASSIGN(TypeName)  \
    TypeName(const TypeName&);               \
    TypeName& operator=(const TypeName&);
#else
# define DISALLOW_COPY_AND_ASSIGN(TypeName)       \
    TypeName(const TypeName&) = delete;           \
    TypeName& operator=(const TypeName&) = delete;
#endif

#define ATTRIBUTE_UNUSED __attribute__((unused))
#ifdef NDEBUG
# define ATTRIBUTE_DEBUG_UNUSED ATTRIBUTE_UNUSED
#else
# define ATTRIBUTE_DEBUG_UNUSED
#endif

#define ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#define ATTRIBUTE_NOINLINE __attribute__((noinline))
#if __cplusplus < 201103L
# define ATTRIBUTE_NORETURN __attribute__((noreturn))
#else
# define ATTRIBUTE_NORETURN [[noreturn]]
#endif

#if __has_feature(cxx_thread_local) \
    || (!defined(__clang__) && __cplusplus >= 201103L)
# define ATTRIBUTE_THREAD_LOCAL thread_local
#else
# define ATTRIBUTE_THREAD_LOCAL __thread
#endif

#if defined(__CYGWIN__)
# define OS_CYGWIN 1
#elif defined(_WIN32)
# define OS_WINDOWS 1
#elif defined(__APPLE__)
// TODO(ericwf) This doesn't actually check that it is a Mac OSX system. Just
// that it is an apple system.
# define OS_MACOSX 1
#elif defined(__FreeBSD__)
# define OS_FREEBSD 1
#elif defined(__linux__)
# define OS_LINUX 1
#endif

#endif  // BENCHMARK_MACROS_H_
