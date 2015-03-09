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
#include "benchmark/minimal_benchmark.h"
#include "benchmark/benchmark.h"
#include "check.h"

namespace benchmark {

MinimalBenchmark::MinimalBenchmark(const char* name, Function* f)
  : imp_(new internal::Benchmark(name, f))
{ }

MinimalBenchmark::~MinimalBenchmark() {
  CHECK(imp_ == nullptr);
}

MinimalBenchmark& MinimalBenchmark::Arg(int x) {
  imp_->Arg(x);
  return *this;
}

MinimalBenchmark& MinimalBenchmark::Range(int start, int limit) {
  imp_->Range(start, limit);
  return *this;
}

MinimalBenchmark& MinimalBenchmark::DenseRange(int start, int limit) {
  imp_->DenseRange(start, limit);
  return *this;
}

MinimalBenchmark& MinimalBenchmark::ArgPair(int x, int y)
{
  imp_->ArgPair(x, y);
  return *this;
}

MinimalBenchmark& MinimalBenchmark::RangePair(int lo1, int hi1, int lo2,
                                              int hi2) {
  imp_->RangePair(lo1, hi1, lo2, hi2);
  return *this;
}

MinimalBenchmark& MinimalBenchmark::Apply(void (*func)(internal::Benchmark*)) {
  imp_->Apply(func);
  return *this;
}

MinimalBenchmark& MinimalBenchmark::Threads(int t) {
  imp_->Threads(t);
  return *this;
}

MinimalBenchmark& MinimalBenchmark::ThreadRange(int min_threads,
                                                int max_threads) {
  imp_->ThreadRange(min_threads, max_threads);
  return *this;
}

MinimalBenchmark& MinimalBenchmark::ThreadPerCpu() {
  imp_->ThreadPerCpu();
  return *this;
}

} // end namespace benchmark
