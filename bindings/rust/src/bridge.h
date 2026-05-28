#pragma once

#include "benchmark/benchmark.h"
#include "rust/cxx.h"

#include <cstddef>

// Non-inline C++ wrappers used by the cxx bridge.
//
// benchmark::State::KeepRunning() is marked BENCHMARK_ALWAYS_INLINE so it
// has no linkable symbol.  The functions below provide real exported symbols
// that cxx can call across the FFI boundary.
namespace benchmark_bridge {

void bm_initialize(::rust::Slice<const ::rust::Str> args);
bool bm_keep_running(::benchmark::State& state);
void bm_skip_with_error(::benchmark::State& state, ::rust::Str msg);
void bm_register(::rust::Str name, void (*f)(::benchmark::State&));
::std::size_t bm_run();

}  // namespace benchmark_bridge
