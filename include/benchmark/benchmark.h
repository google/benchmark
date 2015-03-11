// Support for registering benchmarks for functions.

/* Example usage:
// Define a function that executes the code to be measured a
// specified number of times:
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
int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}

// Sometimes a family of microbenchmarks can be implemented with
// just one routine that takes an extra argument to specify which
// one of the family of benchmarks to run.  For example, the following
// code defines a family of microbenchmarks for measuring the speed
// of memcpy() calls of different lengths:

static void BM_memcpy(benchmark::State& state) {
  char* src = new char[state.range_x()]; char* dst = new char[state.range_x()];
  memset(src, 'x', state.range_x());
  while (state.KeepRunning())
    memcpy(dst, src, state.range_x());
  state.SetBytesProcessed(int64_t_t(state.iterations) * int64(state.range_x()));
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
static void BM_SetInsert(benchmark::State& state) {
  while (state.KeepRunning()) {
    state.PauseTiming();
    set<int> data = ConstructRandomSet(state.range_x());
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
static benchmark::internal::Benchmark* CustomArguments(
    benchmark::internal::Benchmark* b) {
  for (int i = 0; i <= 10; ++i)
    for (int j = 32; j <= 1024*1024; j *= 8)
      b = b->ArgPair(i, j);
  return b;
}
BENCHMARK(BM_SetInsert)->Apply(CustomArguments);

// Templated microbenchmarks work the same way:
// Produce then consume 'size' messages 'iters' times
// Measures throughput in the absence of multiprogramming.
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

In a multithreaded test, it is guaranteed that none of the threads will start
until all have called KeepRunning, and all will have finished before KeepRunning
returns false. As such, any global setup or teardown you want to do can be
wrapped in a check against the thread index:

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
BENCHMARK(BM_MultiThreaded)->Threads(4);
*/

#ifndef BENCHMARK_BENCHMARK_H_
#define BENCHMARK_BENCHMARK_H_

#include <stdint.h>

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

#include "macros.h"

namespace benchmark {
class BenchmarkReporter;

void Initialize(int* argc, const char** argv);

// Otherwise, run all benchmarks specified by the --benchmark_filter flag,
// and exit after running the benchmarks.
void RunSpecifiedBenchmarks(const BenchmarkReporter* reporter = nullptr);

// ------------------------------------------------------
// Routines that can be called from within a benchmark

// If this routine is called, peak memory allocation past this point in the
// benchmark is reported at the end of the benchmark report line. (It is
// computed by running the benchmark once with a single iteration and a memory
// tracer.)
// TODO(dominic)
// void MemoryUsage();

// If a particular benchmark is I/O bound, or if for some reason CPU
// timings are not representative, call this method from within the
// benchmark routine.  If called, the elapsed time will be used to
// control how many iterations are run, and in the printing of
// items/second or MB/seconds values.  If not called, the cpu time
// used by the benchmark will be used.
void UseRealTime();

namespace internal {
class Benchmark;
class BenchmarkFamilies;
}

// State is passed to a running Benchmark and contains state for the
// benchmark to use.
class State {
 public:
  // Returns true iff the benchmark should continue through another iteration.
  bool KeepRunning();

  void PauseTiming();
  void ResumeTiming();

  // Set the number of bytes processed by the current benchmark
  // execution.  This routine is typically called once at the end of a
  // throughput oriented benchmark.  If this routine is called with a
  // value > 0, the report is printed in MB/sec instead of nanoseconds
  // per iteration.
  //
  // REQUIRES: a benchmark has exited its KeepRunning loop.
  void SetBytesProcessed(int64_t bytes);

  // If this routine is called with items > 0, then an items/s
  // label is printed on the benchmark report line for the currently
  // executing benchmark. It is typically called at the end of a processing
  // benchmark where a processing items/second output is desired.
  //
  // REQUIRES: a benchmark has exited its KeepRunning loop.
  void SetItemsProcessed(int64_t items);

  // If this routine is called, the specified label is printed at the
  // end of the benchmark report line for the currently executing
  // benchmark.  Example:
  //  static void BM_Compress(benchmark::State& state) {
  //    ...
  //    double compress = input_size / output_size;
  //    state.SetLabel(StringPrintf("compress:%.1f%%", 100.0*compression));
  //  }
  // Produces output that looks like:
  //  BM_Compress   50         50   14115038  compress:27.3%
  //
  // REQUIRES: a benchmark has exited its KeepRunning loop.
  void SetLabel(const std::string& label);

  // Range arguments for this run. CHECKs if the argument has been set.
  int range_x() const;
  int range_y() const;

  int64_t iterations() const { return total_iterations_; }

  const int thread_index;

 private:
  class FastClock;
  struct SharedState;
  struct ThreadStats;

  State(FastClock* clock, SharedState* s, int t);
  bool StartRunning();
  bool FinishInterval();
  bool MaybeStop();
  void NewInterval();
  bool AllStarting();

  static void* RunWrapper(void* arg);
  void Run();
  void RunAsThread();
  void Wait();

  enum EState {
    STATE_INITIAL,   // KeepRunning hasn't been called
    STATE_STARTING,  // KeepRunning called, waiting for other threads
    STATE_RUNNING,   // Running and being timed
    STATE_STOPPING,  // Not being timed but waiting for other threads
    STATE_STOPPED    // Stopped
  };

  EState state_;

  FastClock* clock_;

  // State shared by all BenchmarkRun objects that belong to the same
  // BenchmarkInstance
  SharedState* shared_;

  std::thread thread_;

  // Custom label set by the user.
  std::string label_;

  // Each State object goes through a sequence of measurement intervals. By
  // default each interval is approx. 100ms in length. The following stats are
  // kept for each interval.
  int64_t iterations_;
  double start_cpu_;
  double start_time_;
  int64_t stop_time_micros_;

  double start_pause_cpu_;
  double pause_cpu_time_;
  double start_pause_real_;
  double pause_real_time_;

  // Total number of iterations for all finished runs.
  int64_t total_iterations_;

  // Approximate time in microseconds for one interval of execution.
  // Dynamically adjusted as needed.
  int64_t interval_micros_;

  // True if the current interval is the continuation of a previous one.
  bool is_continuation_;

  std::unique_ptr<ThreadStats> stats_;

  friend class internal::Benchmark;
  BENCHMARK_DISALLOW_COPY_AND_ASSIGN(State);
};

// Interface for custom benchmark result printers.
// By default, benchmark reports are printed to stdout. However an application
// can control the destination of the reports by calling
// RunSpecifiedBenchmarks and passing it a custom reporter object.
// The reporter object must implement the following interface.
class BenchmarkReporter {
 public:
  struct Context {
    int num_cpus;
    double mhz_per_cpu;
    // std::string cpu_info;
    bool cpu_scaling_enabled;

    // The number of chars in the longest benchmark name.
    size_t name_field_width;
  };

  struct Run {
    Run()
        : thread_index(-1),
          iterations(1),
          real_accumulated_time(0),
          cpu_accumulated_time(0),
          bytes_per_second(0),
          items_per_second(0),
          max_heapbytes_used(0) {}

    std::string benchmark_name;
    std::string report_label;
    int thread_index;
    int64_t iterations;
    double real_accumulated_time;
    double cpu_accumulated_time;

    // Zero if not set by benchmark.
    double bytes_per_second;
    double items_per_second;

    // This is set to 0.0 if memory tracing is not enabled.
    double max_heapbytes_used;
  };

  // Called once for every suite of benchmarks run.
  // The parameter "context" contains information that the
  // reporter may wish to use when generating its report, for example the
  // platform under which the benchmarks are running. The benchmark run is
  // never started if this function returns false, allowing the reporter
  // to skip runs based on the context information.
  virtual bool ReportContext(const Context& context) const = 0;

  // Called once for each group of benchmark runs, gives information about
  // cpu-time and heap memory usage during the benchmark run.
  // Note that all the grouped benchmark runs should refer to the same
  // benchmark, thus have the same name.
  virtual void ReportRuns(const std::vector<Run>& report) const = 0;

  virtual ~BenchmarkReporter() {}
};

namespace internal {

typedef std::function<void(State&)> BenchmarkFunction;

// Run all benchmarks whose name is a partial match for the regular
// expression in "spec". The results of benchmark runs are fed to "reporter".
void RunMatchingBenchmarks(const std::string& spec,
                           const BenchmarkReporter* reporter);

// Extract the list of benchmark names that match the specified regular
// expression.
void FindMatchingBenchmarkNames(const std::string& re,
                                std::vector<std::string>* benchmark_names);

// ------------------------------------------------------
// Benchmark registration object.  The BENCHMARK() macro expands
// into an internal::Benchmark* object.  Various methods can
// be called on this object to change the properties of the benchmark.
// Each method returns "this" so that multiple method calls can
// chained into one expression.
class Benchmark {
 public:
  // The Benchmark takes ownership of the Callback pointed to by f.
  Benchmark(const char* name, BenchmarkFunction f);

  ~Benchmark();

  // Note: the following methods all return "this" so that multiple
  // method calls can be chained together in one expression.

  // Run this benchmark once with "x" as the extra argument passed
  // to the function.
  // REQUIRES: The function passed to the constructor must accept an arg1.
  Benchmark* Arg(int x);

  // Run this benchmark once for a number of values picked from the
  // range [start..limit].  (start and limit are always picked.)
  // REQUIRES: The function passed to the constructor must accept an arg1.
  Benchmark* Range(int start, int limit);

  // Run this benchmark once for every value in the range [start..limit]
  // REQUIRES: The function passed to the constructor must accept an arg1.
  Benchmark* DenseRange(int start, int limit);

  // Run this benchmark once with "x,y" as the extra arguments passed
  // to the function.
  // REQUIRES: The function passed to the constructor must accept arg1,arg2.
  Benchmark* ArgPair(int x, int y);

  // Pick a set of values A from the range [lo1..hi1] and a set
  // of values B from the range [lo2..hi2].  Run the benchmark for
  // every pair of values in the cartesian product of A and B
  // (i.e., for all combinations of the values in A and B).
  // REQUIRES: The function passed to the constructor must accept arg1,arg2.
  Benchmark* RangePair(int lo1, int hi1, int lo2, int hi2);

  // Pass this benchmark object to *func, which can customize
  // the benchmark by calling various methods like Arg, ArgPair,
  // Threads, etc.
  Benchmark* Apply(void (*func)(Benchmark* benchmark));

  // Support for running multiple copies of the same benchmark concurrently
  // in multiple threads.  This may be useful when measuring the scaling
  // of some piece of code.

  // Run one instance of this benchmark concurrently in t threads.
  Benchmark* Threads(int t);

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
  Benchmark* ThreadRange(int min_threads, int max_threads);

  // Equivalent to ThreadRange(NumCPUs(), NumCPUs())
  Benchmark* ThreadPerCpu();

  // -------------------------------
  // Following methods are not useful for clients

  // Used inside the benchmark implementation
  struct Instance;

  // Measure the overhead of an empty benchmark to subtract later.
  static void MeasureOverhead();

 private:
  friend class BenchmarkFamilies;

  std::vector<Benchmark::Instance> CreateBenchmarkInstances(size_t rangeXindex,
                                                            size_t rangeYindex);

  std::string name_;
  BenchmarkFunction function_;
  size_t registration_index_;
  std::vector<int> rangeX_;
  std::vector<int> rangeY_;
  std::vector<int> thread_counts_;
  std::mutex mutex_;

  // Special value placed in thread_counts_ to stand for NumCPUs()
  static const int kNumCpuMarker = -1;

  // Special value used to indicate that no range is required.
  static const size_t kNoRangeIndex = std::numeric_limits<size_t>::max();
  static const int kNoRange = std::numeric_limits<int>::max();

  static void AddRange(std::vector<int>* dst, int lo, int hi, int mult);
  static double MeasurePeakHeapMemory(const Instance& b);
  static void RunInstance(const Instance& b, const BenchmarkReporter* br);
  friend class ::benchmark::State;
  friend struct ::benchmark::internal::Benchmark::Instance;
  friend void ::benchmark::internal::RunMatchingBenchmarks(
      const std::string&, const BenchmarkReporter*);
  BENCHMARK_DISALLOW_COPY_AND_ASSIGN(Benchmark);
};

// ------------------------------------------------------
// Internal implementation details follow; please ignore

// Simple reporter that outputs benchmark data to the console. This is the
// default reporter used by RunSpecifiedBenchmarks().
class ConsoleReporter : public BenchmarkReporter {
 public:
  virtual bool ReportContext(const Context& context) const;
  virtual void ReportRuns(const std::vector<Run>& reports) const;

 private:
  std::string PrintMemoryUsage(double bytes) const;
  virtual void PrintRunData(const Run& report) const;
  mutable size_t name_field_width_;
};

}  // end namespace internal
}  // end namespace benchmark

// ------------------------------------------------------
// Macro to register benchmarks

// Helpers for generating unique variable names
#define BENCHMARK_CONCAT(a, b, c) BENCHMARK_CONCAT2(a, b, c)
#define BENCHMARK_CONCAT2(a, b, c) a##b##c

#define BENCHMARK(n)                                         \
  static ::benchmark::internal::Benchmark* BENCHMARK_CONCAT( \
      __benchmark_, n, __LINE__) BENCHMARK_UNUSED =          \
      (new ::benchmark::internal::Benchmark(#n, n))

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
#define BENCHMARK_TEMPLATE(n, a)                             \
  static ::benchmark::internal::Benchmark* BENCHMARK_CONCAT( \
      __benchmark_, n, __LINE__) BENCHMARK_UNUSED =          \
      (new ::benchmark::internal::Benchmark(#n "<" #a ">", n<a>))

#define BENCHMARK_TEMPLATE2(n, a, b)                         \
  static ::benchmark::internal::Benchmark* BENCHMARK_CONCAT( \
      __benchmark_, n, __LINE__) BENCHMARK_UNUSED =          \
      (new ::benchmark::internal::Benchmark(#n "<" #a "," #b ">", n<a, b>))

#endif  // BENCHMARK_BENCHMARK_H_
