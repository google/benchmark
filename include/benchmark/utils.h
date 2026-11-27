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

#ifndef BENCHMARK_UTILS_H_
#define BENCHMARK_UTILS_H_

#include <atomic>
#include <type_traits>
#include <utility>

#include "benchmark/export.h"
#include "benchmark/macros.h"

namespace benchmark {

namespace internal {
BENCHMARK_EXPORT void UseCharPointer(char const volatile*);

// An object of a class type with a const data member has nothing to store
// into, so gcc and clang reject it as an asm *output* operand (issue #1997).
// No trait spells that directly; assignability is the closest approximation.
// Arrays are never assignable but are valid outputs, so allow them outright.
// A class with a reference data member is also a valid output, but is
// indistinguishable here and is conservatively treated as an input instead.
template <class Tp>
struct IsValidAsmOutput {
  typedef typename std::remove_reference<Tp>::type T;
  static const bool value = std::is_array<T>::value ||
                            std::is_copy_assignable<T>::value ||
                            std::is_move_assignable<T>::value;
};
}  // namespace internal

#if (!defined(__GNUC__) && !defined(__clang__)) || defined(__pnacl__) || \
    defined(__EMSCRIPTEN__)
#define BENCHMARK_HAS_NO_INLINE_ASSEMBLY
#endif

inline BENCHMARK_ALWAYS_INLINE void ClobberMemory() {
  std::atomic_signal_fence(std::memory_order_acq_rel);
}

#define BENCHMARK_DONOTOPTIMIZE_CONST_REF_DEPRECATED_MSG                       \
  "DoNotOptimize(T const&) can permit undesired compiler optimizations. "      \
  "Pass a non-const lvalue instead; if the argument is an expression result, " \
  "store it in a local variable first."

#ifndef BENCHMARK_HAS_NO_INLINE_ASSEMBLY
#if !defined(__GNUC__) || defined(__llvm__) || defined(__INTEL_COMPILER)
template <class Tp>
BENCHMARK_DEPRECATED_MSG(BENCHMARK_DONOTOPTIMIZE_CONST_REF_DEPRECATED_MSG)
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsValidAsmOutput<Tp>::value>::type
    DoNotOptimize(Tp& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsValidAsmOutput<Tp>::value>::type
    DoNotOptimize(Tp&& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

// Types that cannot be written to are passed as an input operand instead.
// A forwarding reference covers both lvalues and rvalues here; unlike the
// read-write overloads above, an input operand needs no separate lvalue form.
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!internal::IsValidAsmOutput<Tp>::value>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}
#elif (__GNUC__ >= 5)
template <class Tp>
BENCHMARK_DEPRECATED_MSG(BENCHMARK_DONOTOPTIMIZE_CONST_REF_DEPRECATED_MSG)
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp const& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
BENCHMARK_DEPRECATED_MSG(BENCHMARK_DONOTOPTIMIZE_CONST_REF_DEPRECATED_MSG)
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!std::is_trivially_copyable<Tp>::value ||
                            (sizeof(Tp) > sizeof(Tp*))>::type
    DoNotOptimize(Tp const& value) {
  asm volatile("" : : "m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsValidAsmOutput<Tp>::value &&
                            std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp& value) {
  asm volatile("" : "+m,r"(value) : : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsValidAsmOutput<Tp>::value &&
                            (!std::is_trivially_copyable<Tp>::value ||
                             (sizeof(Tp) > sizeof(Tp*)))>::type
    DoNotOptimize(Tp& value) {
  asm volatile("" : "+m"(value) : : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsValidAsmOutput<Tp>::value &&
                            std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : "+m,r"(value) : : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsValidAsmOutput<Tp>::value &&
                            (!std::is_trivially_copyable<Tp>::value ||
                             (sizeof(Tp) > sizeof(Tp*)))>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : "+m"(value) : : "memory");
}

// Types that cannot be written to are passed as an input operand instead.
// A forwarding reference covers both lvalues and rvalues here; unlike the
// read-write overloads above, an input operand needs no separate lvalue form.
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!internal::IsValidAsmOutput<Tp>::value>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}
#endif

#elif defined(_MSC_VER)
template <class Tp>
BENCHMARK_DEPRECATED_MSG(BENCHMARK_DONOTOPTIMIZE_CONST_REF_DEPRECATED_MSG)
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const& value) {
  internal::UseCharPointer(&reinterpret_cast<char const volatile&>(value));
  ClobberMemory();
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp& value) {
  internal::UseCharPointer(&reinterpret_cast<char const volatile&>(value));
  ClobberMemory();
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp&& value) {
  internal::UseCharPointer(&reinterpret_cast<char const volatile&>(value));
  ClobberMemory();
}
#else
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp&& value) {
  internal::UseCharPointer(&reinterpret_cast<char const volatile&>(value));
}
#endif

#undef BENCHMARK_DONOTOPTIMIZE_CONST_REF_DEPRECATED_MSG

}  // end namespace benchmark

#endif  // BENCHMARK_UTILS_H_
