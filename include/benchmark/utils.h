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

// GCC and Clang reject objects of class types with const data members
// (recursively) as asm *output* operands, since such objects cannot be
// stored to (see issue #1997). There is no portable trait for "contains a
// const member", so approximate it as "not assignable": this additionally
// routes reference-member types (which would be valid outputs) through the
// input-only fallback, which is harmless — the fallback keeps the memory
// clobber, and such objects cannot be legally written through anyway.
// Arrays are not assignable but are valid memory outputs, so they are
// accepted unconditionally.
template <class Tp>
struct IsAsmOutputOperand
    : std::integral_constant<
          bool,
          std::is_array<typename std::remove_reference<Tp>::type>::value ||
              std::is_copy_assignable<
                  typename std::remove_reference<Tp>::type>::value ||
              std::is_move_assignable<
                  typename std::remove_reference<Tp>::type>::value> {};
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
    typename std::enable_if<internal::IsAsmOutputOperand<Tp>::value>::type
    DoNotOptimize(Tp& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

// Non-assignable types (e.g. with const or reference data members) cannot be
// used as an asm output operand. Fall back to an input operand with a memory
// clobber; such objects cannot be written through anyway, so no optimization
// barrier is lost.
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!internal::IsAsmOutputOperand<Tp>::value>::type
    DoNotOptimize(Tp& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsAsmOutputOperand<Tp>::value>::type
    DoNotOptimize(Tp&& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!internal::IsAsmOutputOperand<Tp>::value>::type
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
    typename std::enable_if<internal::IsAsmOutputOperand<Tp>::value &&
                            std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp& value) {
  asm volatile("" : "+m,r"(value) : : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsAsmOutputOperand<Tp>::value &&
                            (!std::is_trivially_copyable<Tp>::value ||
                             (sizeof(Tp) > sizeof(Tp*)))>::type
    DoNotOptimize(Tp& value) {
  asm volatile("" : "+m"(value) : : "memory");
}

// Non-assignable types (e.g. with const or reference data members) cannot be
// used as an asm output operand. Fall back to an input operand with a memory
// clobber; such objects cannot be written through anyway, so no optimization
// barrier is lost.
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!internal::IsAsmOutputOperand<Tp>::value &&
                            std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!internal::IsAsmOutputOperand<Tp>::value &&
                            (!std::is_trivially_copyable<Tp>::value ||
                             (sizeof(Tp) > sizeof(Tp*)))>::type
    DoNotOptimize(Tp& value) {
  asm volatile("" : : "m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsAsmOutputOperand<Tp>::value &&
                            std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : "+m,r"(value) : : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<internal::IsAsmOutputOperand<Tp>::value &&
                            (!std::is_trivially_copyable<Tp>::value ||
                             (sizeof(Tp) > sizeof(Tp*)))>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : "+m"(value) : : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!internal::IsAsmOutputOperand<Tp>::value &&
                            std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!internal::IsAsmOutputOperand<Tp>::value &&
                            (!std::is_trivially_copyable<Tp>::value ||
                             (sizeof(Tp) > sizeof(Tp*)))>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : : "m"(value) : "memory");
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
