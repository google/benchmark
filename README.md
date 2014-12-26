benchmark
=========
[![Build Status](https://travis-ci.org/google/benchmark.svg?branch=master)](https://travis-ci.org/google/benchmark)

A library to support the benchmarking of functions, similar to unit-tests.

Discussion group: https://groups.google.com/d/forum/benchmark-discuss

Example usage
-------------
Define a function that executes the code to be measured a
specified number of times:

```c++
static void BM_StringCreation(benchmark::State& state) {
  while (state.KeepRunning())
    std::string empty_string;
}
// Register the function as a benchmark
BENCHMARK(BM_StringCreation);

// Define another benchmark
static void BM_StringCopy(benchmark::State& state) {
  std::string x = "hello";
  while (state.KeepRunning())
    std::string copy(x);
}
BENCHMARK(BM_StringCopy);

// Augment the main() program to invoke benchmarks if specified
// via the --benchmarks command line flag.  E.g.,
//       my_unittest --benchmark_filter=all
//       my_unittest --benchmark_filter=BM_StringCreation
//       my_unittest --benchmark_filter=String
//       my_unittest --benchmark_filter='Copy|Creation'
int main(int argc, const char* argv[]) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}
```

Sometimes a family of microbenchmarks can be implemented with
just one routine that takes an extra argument to specify which
one of the family of benchmarks to run.  For example, the following
code defines a family of microbenchmarks for measuring the speed
of `memcpy()` calls of different lengths:

```c++
static void BM_memcpy(benchmark::State& state) {
  char* src = new char[state.range_x()]; char* dst = new char[state.range_x()];
  memset(src, 'x', state.range_x());
  while (state.KeepRunning())
    memcpy(dst, src, state.range_x());
  state.SetBytesProcessed(int64_t(state.iterations) * int64_t(state.range_x()));
  delete[] src;
  delete[] dst;
}
BENCHMARK(BM_memcpy)->Arg(8)->Arg(64)->Arg(512)->Arg(1<<10)->Arg(8<<10);
```

The preceding code is quite repetitive, and can be replaced with the
following short-hand.  The following invocation will pick a few
appropriate arguments in the specified range and will generate a
microbenchmark for each such argument.

```c++
BENCHMARK(BM_memcpy)->Range(8, 8<<10);
```

You might have a microbenchmark that depends on two inputs.  For
example, the following code defines a family of microbenchmarks for
measuring the speed of set insertion.

```c++
static void BM_SetInsert(benchmark::State& state) {
  while (state.KeepRunning()) {
    state.PauseTiming();
    std::set<int> data = ConstructRandomSet(state.range_x());
    state.ResumeTiming();
    for (int j = 0; j < state.rangeY; ++j)
      data.insert(RandomNumber());
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
```

The preceding code is quite repetitive, and can be replaced with
the following short-hand.  The following macro will pick a few
appropriate arguments in the product of the two specified ranges
and will generate a microbenchmark for each such pair.

```c++
BENCHMARK(BM_SetInsert)->RangePair(1<<10, 8<<10, 1, 512);
```

For more complex patterns of inputs, passing a custom function
to Apply allows programmatic specification of an
arbitrary set of arguments to run the microbenchmark on.
The following example enumerates a dense range on one parameter,
and a sparse range on the second.

```c++
static benchmark::internal::Benchmark* CustomArguments(
    benchmark::internal::Benchmark* b) {
  for (int i = 0; i <= 10; ++i)
    for (int j = 32; j <= 1024*1024; j *= 8)
      b = b->ArgPair(i, j);
  return b;
}
BENCHMARK(BM_SetInsert)->Apply(CustomArguments);
```

Templated microbenchmarks work the same way:
Produce then consume 'size' messages 'iters' times
Measures throughput in the absence of multiprogramming.

```c++
template <class Q> int BM_Sequential(benchmark::State& state) {
  Q q;
  typename Q::value_type v;
  while (state.KeepRunning()) {
    for (int i = state.range_x(); i--; )
      q.push(v);
    for (int e = state.range_x(); e--; )
      q.Wait(&v);
  }
  // actually messages, not bytes:
  state.SetBytesProcessed(
      static_cast<int64_t>(state.iterations())*state.range_x());
}
BENCHMARK_TEMPLATE(BM_Sequential, WaitQueue<int>)->Range(1<<0, 1<<10);
```

In a multithreaded test, it is guaranteed that none of the threads will start
until all have called KeepRunning, and all will have finished before KeepRunning
returns false. As such, any global setup or teardown you want to do can be
wrapped in a check against the thread index:

```c++
static void BM_MultiThreaded(benchmark::State& state) {
  if (state.thread_index == 0) {
    // Setup code here.
  }
  while (state.KeepRunning()) {
    // Run the test as normal.
  }
  if (state.thread_index == 0) {
    // Teardown code here.
  }
}
BENCHMARK(BM_MultiThreaded)->Threads(2);
```

Linking against the library
---------------------------
When using gcc, it is necessary to link against pthread to avoid runtime exceptions. This is due to how gcc implements std::thread. See [issue #67](https://github.com/google/benchmark/issues/67) for more details.
