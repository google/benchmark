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
/*

// Define a function that executes the code to be measured a
// specified number of times:
static void BM_StringCreation(int n) {
  while (n-- > 0) {
    string empty_string;
  }
}

// Register the function as a benchmark
BENCHMARK(BM_StringCreation);

// Define another benchmark
static void BM_StringCopy(int n) {
  string x = "hello";
  while (n-- > 0) {
    string copy(x);
  }
}
BENCHMARK(BM_StringCopy);

// Augment the main() program to invoke benchmarks if specified
// via the --benchmarks command line flag.  E.g.,
//       my_unittest --benchmarks=all
//       my_unittest --benchmarks=BM_StringCreation
//       my_unittest --benchmarks=String
//       my_unittest --benchmarks='Copy|Creation'
int main(int argc, char** argv) {
  InitGoogle(argv[0], &argc, &argv, true);

  RunSpecifiedBenchmarks(); // Does nothing unless --benchmarks is specified

  ... normal unittest code ...;
}

// Sometimes a family of microbenchmarks can be implemented with
// just one routine that takes an extra argument to specify which
// one of the family of benchmarks to run.  For example, the following
// code defines a family of microbenchmarks for measuring the speed
// of memcpy() calls of different lengths:

static void BM_memcpy(int iters, int nbytes) {
  char* src = new char[nbytes]; char* dst = new char[nbytes];
  memset(src, 'x', nbytes);
  for (int i = 0; i < iters; i++) {
    memcpy(dst, src, nbytes);
  }
  SetBenchmarkBytesProcessed(int64_t(iters) * int64_t(nbytes));
  delete[] src; delete[] dst;
}
BENCHMARK(BM_memcpy)->Arg(8)->Arg(64)->Arg(512)->Arg(1<<10)->Arg(8<<10);

// The preceding code is quite repetitive, and can be replaced with the
// following short-hand.  The following invocation will pick a few
// appropriate arguments in the specified range and will generate a
// microbenchmark for each such argument.
BENCHMARK(BM_memcpy)->Range(8, 8<<10);

// You might have a microbenchmark that depends on two inputs.  For
// example, the following code defines a family of microbenchmarks for
// measuring the speed of set insertion.
static void BM_SetInsert(int iters, int size, int how_many) {
  for (int i = 0; i < iters; ++i) {
    StopBenchmarkTiming();
    set<int> data = ConstructRandomSet(size);
    StartBenchmarkTiming();
    for (int j = 0; j < how_many; ++j) {
      data.insert(RandomNumber());
    }
  }
}
BENCHMARK(BM_SetInsert)
   ->ArgPair(1<<10, 1)
   ->ArgPair(1<<10, 8)
   ->ArgPair(1<<10, 64)
   ->ArgPair(1<<10, 512)
   ->ArgPair(8<<10, 1)
   ->ArgPair(8<<10, 8)
   ->ArgPair(8<<10, 64)
   ->ArgPair(8<<10, 512);

// The preceding code is quite repetitive, and can be replaced with
// the following short-hand.  The following macro will pick a few
// appropriate arguments in the product of the two specified ranges
// and will generate a microbenchmark for each such pair.
BENCHMARK(BM_SetInsert)->RangePair(1<<10, 8<<10, 1, 512);

// For more complex patterns of inputs, passing a custom function
// to Apply allows programmatic specification of an
// arbitrary set of arguments to run the microbenchmark on.
// The following example enumerates a dense range on
// one parameter, and a sparse range on the second.
static void CustomArguments(benchmark::Benchmark* b) {
  for (int i = 0; i <= 10; ++i) {
    for (int j = 32; j <= 1024*1024; j *= 8) {
      b = b->ArgPair(i, j);
    }
  }
}
BENCHMARK(BM_SetInsert)->Apply(CustomArguments);

// Templated microbenchmarks work the same way:
// Produce then consume 'size' messages 'iters' times
// Measures throughput in the absence of multiprogramming.
template <class Q> void BM_Sequential(int iters, int size) {
  Q q;
  typename Q::value_type v;
  for (int r = iters; r--; ) {
    for (int i = size; i--; ) {
      q.push(v);
    }
    for (int e = size; e--; ) {
      q.Wait(&v);
    }
  }
  // actually messages, not bytes:
  SetBenchmarkBytesProcessed(static_cast<int64_t>(iters)*size);
}
BENCHMARK_TEMPLATE(BM_Sequential, WaitQueue<int>)->Range(1<<0, 1<<10);

You can use your own Setup and Teardown hooks. Useful mainly for multi-threaded
benchmarks.

static void MemcpySetup(int num_threads);
static void MemcpyTeardown(int num_threads);

BENCHMARK(BM_memcpy)->Setup(NewPermanentCallback(MemcpySetup))
                    ->Teardown(NewPermanentCallback(MemcpyTeardown));

*/

#ifndef BENCHMARK_MINIMAL_BENCHMARK_H_
#define BENCHMARK_MINIMAL_BENCHMARK_H_

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "macros.h"

namespace benchmark {

class BenchmarkReporter;

// Initialize the package and parse the command line flags.
// This function must be called before using the bennchmark library.
extern void Initialize(int* argc, const char** argv);

// If the --benchmarks flag is empty, do nothing.
//
// Otherwise, run all benchmarks specified by the --benchmarks flag,
// and exit after running the benchmarks.
extern void RunSpecifiedBenchmarks(BenchmarkReporter* reporter = NULL);

// ------------------------------------------------------
// Routines that can be called from within a benchmark

// Set the number of bytes processed by the current benchmark
// execution.  This routine is typically called once at the end of a
// throughput oriented benchmark.  If this routine is called with a
// value > 0, the report is printed in MB/sec instead of nanoseconds
// per iteration.  By default this uses CPU seconds when computing rates.
// Use BenchmarkUseRealTime() if you want rates in terms of wallclock time
// instead.
//
// REQUIRES: a benchmark is currently executing
extern void SetBenchmarkBytesProcessed(int64_t bytes);

// If this routine is called with items > 0, then an items/s
// label is printed on the benchmark report line for the currently
// executing benchmark. It is typically called at the end of a processing
// benchmark where a processing items/second output is desired.
// By default this uses CPU seconds when computing rates. Use
// BenchmarkUseRealTime() if you want rates in terms of wallclock time instead.
//
// REQUIRED: a benchmark is currently executing
extern void SetBenchmarkItemsProcessed(int64_t items);

// If this routine is called, the specified label is printed at the
// end of the benchmark report line for the currently executing
// benchmark.  Example:
//  static void BM_Compress(int iters) {
//    ...
//    double compress = input_size / output_size;
//    SetBenchmarkLabel(StringPrintf("compress:%.1f%%", 100.0*compression));
//  }
// Produces output that looks like:
//  BM_Compress   50         50   14115038  compress:27.3%
//
// REQUIRES: a benchmark is currently executing
extern void SetBenchmarkLabel(const char* label);

// REQUIRES: timer is not running
// Start the benchmark timer.  The timer is running on entrance to the
// benchmark function.
//
// For threaded benchmarks the StartBenchmarkTiming() function acts
// like a barrier.  I.e., the ith call by a particular thread to this
// function will block until all threads have made their ith call.
// The timer will start when the last thread has called this function.
//
// NOTE: StartBenchmarkTiming()/StopBenchmarkTiming() are relatively
// heavyweight, and so their use should generally be avoided
// within each benchmark iteration, if possible.
extern void StartBenchmarkTiming();

// REQUIRES: timer is running
// Stop the benchmark timer.  If not called, the timer will be
// automatically stopped after the benchmark function returns.
//
// For threaded benchmarks the StopBenchmarkTiming() function acts
// like a barrier.  I.e., the ith call by a particular thread to this
// function will block until all threads have made their ith call.
// The timer will stop when the last thread has called this function.
//
// NOTE: StartBenchmarkTiming()/StopBenchmarkTiming() are relatively
// heavyweight, and so their use should generally be avoided
// within each benchmark iteration, if possible.
extern void StopBenchmarkTiming();

// Backwards compatible shortcut for
// BenchmarkMemoryUsage(benchmark::kPerThreadUsage).
extern void BenchmarkMemoryUsage();

// If a particular benchmark is I/O bound, or if for some reason CPU
// timings are not representative, call this method from within the
// benchmark routine.  If called, the elapsed time will be used to
// control how many iterations are run, and in the printing of
// items/second or MB/seconds values.  If not called, the cpu time
// used by the benchmark will be used.
extern void BenchmarkUseRealTime();


// ------------------------------------------------------
// Benchmark registration object.  The BENCHMARK() macro expands
// into a benchmark::Benchmark* object.  Various methods can
// be called on this object to change the properties of the benchmark.
// Each method returns "this" so that multiple method calls can
// chained into one expression.

class Benchmark;


// State is passed to a running Benchmark and contains state for the
// benchmark to use.
class State {
public:
  State(int max_iters, bool has_x, int x, bool has_y, int y, int thread_i)
    : started_(false), total_iterations_(0), max_iterations_(max_iters),
      has_range_x_(has_x), range_x_(x),
      has_range_y_(has_y), range_y_(y),
      thread_index(thread_i)
  {}

  // Returns true iff the benchmark should continue through another iteration.
  ATTRIBUTE_ALWAYS_INLINE
  bool KeepRunning() {
    if (__builtin_expect(!started_, false)) {
        StartBenchmarkTiming();
        started_ = true;
    }
    bool const res = total_iterations_++ < max_iterations_;
    if (__builtin_expect(!res, false)) {
        assert(started_);
        StopBenchmarkTiming();
    }
    return res;
  }

  ATTRIBUTE_ALWAYS_INLINE
  void PauseTiming() {
    StopBenchmarkTiming();
  }

  ATTRIBUTE_ALWAYS_INLINE
  void ResumeTiming() {
    StartBenchmarkTiming();
  }

  // Set the number of bytes processed by the current benchmark
  // execution.  This routine is typically called once at the end of a
  // throughput oriented benchmark.  If this routine is called with a
  // value > 0, the report is printed in MB/sec instead of nanoseconds
  // per iteration.
  //
  // REQUIRES: a benchmark has exited its KeepRunning loop.
  ATTRIBUTE_ALWAYS_INLINE
  void SetBytesProcessed(int64_t bytes) {
    SetBenchmarkBytesProcessed(bytes);
  }

  // If this routine is called with items > 0, then an items/s
  // label is printed on the benchmark report line for the currently
  // executing benchmark. It is typically called at the end of a processing
  // benchmark where a processing items/second output is desired.
  //
  // REQUIRES: a benchmark has exited its KeepRunning loop.
  ATTRIBUTE_ALWAYS_INLINE
  void SetItemsProcessed(int64_t items) {
    SetBenchmarkItemsProcessed(items);
  }

  // If this routine is called, the specified label is printed at the
  // end of the benchmark report line for the currently executing
  // benchmark.  Example:
  //  static void BM_Compress(int iters) {
  //    ...
  //    double compress = input_size / output_size;
  //    benchmark::SetLabel(StringPrintf("compress:%.1f%%", 100.0*compression));
  //  }
  // Produces output that looks like:
  //  BM_Compress   50         50   14115038  compress:27.3%
  //
  // REQUIRES: a benchmark has exited its KeepRunning loop.
  ATTRIBUTE_ALWAYS_INLINE
  void SetLabel(const char* label) {
    SetBenchmarkLabel(label);
  }

  // Range arguments for this run. CHECKs if the argument has been set.
  ATTRIBUTE_ALWAYS_INLINE
  int range_x() const {
    assert(has_range_x_);
    return range_x_;
  }

  ATTRIBUTE_ALWAYS_INLINE
  int range_y() const {
    assert(has_range_y_);
    return range_y_;
  }

  ATTRIBUTE_ALWAYS_INLINE
  int iterations() const { return total_iterations_; }

  ATTRIBUTE_ALWAYS_INLINE
  int max_iterations() const { return max_iterations_; }


private:
    bool started_;
    unsigned total_iterations_, max_iterations_;

    bool has_range_x_;
    int range_x_;

    bool has_range_y_;
    int range_y_;

public:
    const int thread_index;

private:
    DISALLOW_COPY_AND_ASSIGN(State)
};

typedef void(Function)(State&);

class MinimalBenchmark
{
public:
  MinimalBenchmark(const char* name, Function* ptr);

  ~MinimalBenchmark();

  // Note: the following methods all return "this" so that multiple
  // method calls can be chained together in one expression.

  // Run this benchmark once with "x" as the extra argument passed
  // to the function.
  // REQUIRES: The function passed to the constructor must accept an arg1.
  MinimalBenchmark& Arg(int x);

  // Run this benchmark once for a number of values picked from the
  // range [start..limit].  (start and limit are always picked.)
  // REQUIRES: The function passed to the constructor must accept an arg1.
  MinimalBenchmark& Range(int start, int limit);

  // Run this benchmark once for every value in the range [start..limit]
  // REQUIRES: The function passed to the constructor must accept an arg1.
  MinimalBenchmark& DenseRange(int start, int limit);

  // Run this benchmark once with "x,y" as the extra arguments passed
  // to the function.
  // REQUIRES: The function passed to the constructor must accept arg1,arg2.
  MinimalBenchmark& ArgPair(int x, int y);

  // Pick a set of values A from the range [lo1..hi1] and a set
  // of values B from the range [lo2..hi2].  Run the benchmark for
  // every pair of values in the cartesian product of A and B
  // (i.e., for all combinations of the values in A and B).
  // REQUIRES: The function passed to the constructor must accept arg1,arg2.
  MinimalBenchmark& RangePair(int lo1, int hi1, int lo2, int hi2);

  // Pass this benchmark object to *func, which can customize
  // the benchmark by calling various methods like Arg, ArgPair,
  // Threads, etc.
  MinimalBenchmark& Apply(void (*func)(Benchmark* benchmark));

  // Support for running multiple copies of the same benchmark concurrently
  // in multiple threads.  This may be useful when measuring the scaling
  // of some piece of code.

  // Run one instance of this benchmark concurrently in t threads.
  MinimalBenchmark& Threads(int t);

  // Pick a set of values T from [min_threads,max_threads].
  // min_threads and max_threads are always included in T.  Run this
  // benchmark once for each value in T.  The benchmark run for a
  // particular value t consists of t threads running the benchmark
  // function concurrently.  For example, consider:
  //    BENCHMARK(Foo)->ThreadRange(1,16);
  // This will run the following benchmarks:
  //    Foo in 1 thread
  //    Foo in 2 threads
  //    Foo in 4 threads
  //    Foo in 8 threads
  //    Foo in 16 threads
  MinimalBenchmark& ThreadRange(int min_threads, int max_threads);

  // Equivalent to ThreadRange(NumCPUs(), NumCPUs())
  MinimalBenchmark& ThreadPerCpu();

  MinimalBenchmark* operator->() {
    return this;
  }
  operator Benchmark*() {
    Benchmark *tmp = imp_;
    imp_ = nullptr;
    return tmp;
  }

private:
  Benchmark *imp_;
  DISALLOW_COPY_AND_ASSIGN(MinimalBenchmark)
};


}  // end namespace benchmark

// ------------------------------------------------------
// Macro to register benchmarks

// Helpers for generating unique variable names
#define BENCHMARK_CONCAT(a, b, c) BENCHMARK_CONCAT2(a, b, c)
#define BENCHMARK_CONCAT2(a, b, c) a ## b ## c

#define BENCHMARK(n)                                                    \
  static ::benchmark::Benchmark*                                          \
  BENCHMARK_CONCAT(_benchmark_, n, __LINE__) ATTRIBUTE_UNUSED =        \
  (::benchmark::MinimalBenchmark(#n, &n))

// Old-style macros
#define BENCHMARK_WITH_ARG(n, a) BENCHMARK(n)->Arg((a))
#define BENCHMARK_WITH_ARG2(n, a1, a2) BENCHMARK(n)->ArgPair((a1), (a2))
#define BENCHMARK_RANGE(n, lo, hi) BENCHMARK(n)->Range((lo), (hi))
#define BENCHMARK_RANGE2(n, l1, h1, l2, h2) \
  BENCHMARK(n)->RangePair((l1), (h1), (l2), (h2))

// This will register a benchmark for a templatized function.  For example:
//
// template<int arg>
// void BM_Foo(int iters);
//
// BENCHMARK_TEMPLATE(BM_Foo, 1);
//
// will register BM_Foo<1> as a benchmark.
// NOTE: In C++11 BENCHMARK_TEMPLATE is variadic and will accept 1 or more
// template arguments.
#if __cplusplus < 201103L
# define BENCHMARK_TEMPLATE(n, a)                                           \
    static ::benchmark::Benchmark*                                          \
    BENCHMARK_CONCAT(_benchmark_, n, __LINE__) ATTRIBUTE_UNUSED =           \
    (::benchmark::MinimalBenchmark(#n "<" #a ">", &n<a>))
#else
# define BENCHMARK_TEMPLATE(n, ...)                                           \
    static ::benchmark::Benchmark*                                            \
    BENCHMARK_CONCAT(_benchmark_, n, __LINE__) ATTRIBUTE_UNUSED =             \
    (::benchmark::MinimalBenchmark(#n "<" #__VA_ARGS__ ">", &n<__VA_ARGS__>))
#endif

#define BENCHMARK_TEMPLATE2(n, a, b)                                      \
  static ::benchmark::Benchmark*                                          \
  BENCHMARK_CONCAT(__benchmark_, n, __LINE__) ATTRIBUTE_UNUSED =          \
  (::benchmark::MinimalBenchmark(#n "<" #a "," #b ">",                    \
                               &n<a, b>))

// Helper macro to create a main routine in a test that runs the benchmarks
#define BENCHMARK_MAIN()                             \
  int main(int argc, const char** argv) {            \
    ::benchmark::Initialize(&argc, argv);            \
    ::benchmark::RunSpecifiedBenchmarks();           \
  }

#endif // BENCHMARK_MINIMAL_BENCHMARK_H_
