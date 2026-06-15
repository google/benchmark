#pragma once

#include "benchmark/benchmark.h"
#include "rust/cxx.h"

namespace benchmark {
namespace rust_api {

void RegisterBenchmark(rust::Str name, rust::Fn<void(benchmark::State&)> func);
void Initialize(int* argc, size_t argv);
void SkipWithError(benchmark::State& state, rust::Str msg);

}  // namespace rust_api
}  // namespace benchmark
