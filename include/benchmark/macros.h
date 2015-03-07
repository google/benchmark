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

#ifndef BENCHMARK_MACROS_H_
#define BENCHMARK_MACROS_H_

#include <assert.h>
#include <stddef.h>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&);

// The arraysize(arr) macro returns the # of elements in an array arr.
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays, for example.  If you use arraysize on
// a pointer by mistake, you will get a compile-time error.
//
// One caveat is that, for C++03, arraysize() doesn't accept any array of
// an anonymous type or a type defined inside a function.  In these rare
// cases, you have to use the unsafe ARRAYSIZE() macro below.  This is
// due to a limitation in C++03's template system.  The limitation has
// been removed in C++11.

// This template function declaration is used in defining arraysize.
// Note that the function doesn't need an implementation, as we only
// use its type.
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

// That gcc wants both of these prototypes seems mysterious. VC, for
// its part, can't decide which to use (another mystery). Matching of
// template overloads: the final frontier.
#ifndef COMPILER_MSVC
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];
#endif

#define arraysize(array) (sizeof(ArraySizeHelper(array)))

//
// Prevent the compiler from complaining about or optimizing away variables
// that appear unused.
#define ATTRIBUTE_UNUSED __attribute__((unused))

//
// For functions we want to force inline or not inline.
// Introduced in gcc 3.1.
#define ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#define HAVE_ATTRIBUTE_ALWAYS_INLINE 1
#define ATTRIBUTE_NOINLINE __attribute__((noinline))
#define HAVE_ATTRIBUTE_NOINLINE 1

#endif  // BENCHMARK_MACROS_H_
