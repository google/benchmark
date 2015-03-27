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

BENCHMARK_MAIN();
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

Three macros are provided for adding benchmark templates.

```c++
#if __cplusplus >= 201103L // C++11 and greater.
#define BENCHMARK_TEMPLATE(func, ...) // Takes any number of parameters.
#else // C++ < C++11
#define BENCHMARK_TEMPLATE(func, arg1)
#endif
#define BENCHMARK_TEMPLATE1(func, arg1)
#define BENCHMARK_TEMPLATE2(func, arg1, arg2)
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

To prevent a value or expression from being optimized away by the compiler
the `benchmark::DoNotOptimize(...)` function can be used.

```c++
static void BM_test(benchmark::State& state) {
  while (state.KeepRunning()) {
      int x = 0;
      for (int i=0; i < 64; ++i) {
        benchmark::DoNotOptimize(x += i);
      }
  }
}
```


Output Formats
--------------
The library supports multiple output formats. Use the
`--benchmark_format=<tabular|json>` flag to set the format type. `tabular` is
the default format.

The Tabular format is intended to be a human readable
format. By default the format generates color output. Example tabular output
looks like:
```
Run on (40 X 2801 MHz CPUs)
2015/03/17-18:35:54
Build Type: DEBUG
Benchmark                               Time(ns)    CPU(ns) Iterations
----------------------------------------------------------------------
BM_SetInsert/1024/1                        28928      29349      23853  133.097kB/s   33.2742k items/s
BM_SetInsert/1024/8                        32065      32913      21375  949.487kB/s   237.372k items/s
BM_SetInsert/1024/10                       33157      33648      21431  1.13369MB/s   290.225k items/s
```

The JSON format outputs human readable json split into two top level attributes.
The `context` attribute contains information about the run in general, including
information about the CPU and the date.
The `benchmarks` attribute contains a list of ever benchmark run. Example json
output looks like:
```
{
  "context": {
    "date": "2015/03/17-18:40:25",
    "num_cpus": 40,
    "mhz_per_cpu": 2801,
    "cpu_scaling_enabled": false,
    "build_type": "debug"
  },
  "benchmarks": [
    {
      "name": "BM_SetInsert/1024/1",
      "iterations": 94877,
      "real_time": 29275,
      "cpu_time": 29836,
      "bytes_per_second": 134066,
      "items_per_second": 33516
    },
    {
      "name": "BM_SetInsert/1024/8",
      "iterations": 21609,
      "real_time": 32317,
      "cpu_time": 32429,
      "bytes_per_second": 986770,
      "items_per_second": 246693
    },
    {
      "name": "BM_SetInsert/1024/10",
      "iterations": 21393,
      "real_time": 32724,
      "cpu_time": 33355,
      "bytes_per_second": 1199226,
      "items_per_second": 299807
    }
  ]
}
```


Linking against the library
---------------------------
When using gcc, it is necessary to link against pthread to avoid runtime exceptions. This is due to how gcc implements std::thread. See [issue #67](https://github.com/google/benchmark/issues/67) for more details.
