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

/* This file implements FunctionBenchmark and FixtureBenchmark */

#include <memory>
#include "benchmark/benchmark.h"
#include "benchmark_api_internal.h"

namespace {
using namespace benchmark;
using namespace benchmark::internal;

//=============================================================================//
//                            FunctionBenchmark
//=============================================================================//

// The class used to hold all Benchmarks created from static function.
// (ie those created using the BENCHMARK(...) macros.
class FunctionBenchmark : public Benchmark {
 public:
  FunctionBenchmark(const char* name, Function* func)
      : Benchmark(name), func_(func) {}

  void Run(State& st) override { func_(st); }

 private:
  Function* func_;
};

//=============================================================================//
//                            FixtureBenchmark
//=============================================================================//
//
// The class used to hold all Benchmarks created from fixture.
// (ie those created using the BENCHMARK_F(...) macros.
class FixtureBenchmark : public Benchmark {
 public:
  FixtureBenchmark(const char* name, FixtureCreator* creator)
      : Benchmark(name), creator_(creator) {}

  void Run(State& st) override {
    fixture_->SetUp(st);
    fixture_->BenchmarkCase(st);
    fixture_->TearDown(st);
  }

 private:
  void Init() override {
    if (!fixture_) {
      fixture_.reset(creator_());
    }
  }

  std::unique_ptr<Fixture> fixture_;
  FixtureCreator* creator_;
  friend BenchmarkFamilies;
};

}  // namespace

namespace benchmark {

internal::Benchmark* RegisterBenchmark(const char* name,
                                       internal::Function* func) {
  return RegisterBenchmarkInternal(new FunctionBenchmark(name, func));
}

namespace internal {

Benchmark* RegisterBenchmark(const char* name, FixtureCreator* creator) {
  return RegisterBenchmarkInternal(new FixtureBenchmark(name, creator));
}

}  // namespace internal
}  // namespace benchmark

