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
/*

  See minimal_benchmark.h for documentation and example usage.

  minimal_benchmark.h provides almost all of the library API without any
  dependencies on the standard library. In order to benchmark the standard
  library it is desirable to minimize dependency on the standard library.

*/
#ifndef BENCHMARK_BENCHMARK_H_
#define BENCHMARK_BENCHMARK_H_

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "macros.h"
#include "minimal_benchmark.h"

namespace benchmark {

inline void SetLabel(std::string const& label) {
    SetLabel(label.c_str());
}

class Benchmark {
 public:
  // The Benchmark takes ownership of the Callback pointed to by f.
  Benchmark(const std::string& name, Function* f);

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

  // Extract the list of benchmark instances that match the specified
  // regular expression.
  static void FindBenchmarks(const std::string& re,
                             std::vector<Instance>* benchmarks);

 private:
  std::string name_;
  Function* function_;
  std::size_t registration_index_;
  int arg_count_;
  std::vector< std::pair<int, int> > args_;  // Args for all benchmark runs
  std::vector<int> thread_counts_;

  // Special value placed in thread_counts_ to stand for NumCPUs()
  static const int kNumCpuMarker = -1;

  static void AddRange(std::vector<int>* dst, int lo, int hi, int mult);

  BENCHMARK_DISALLOW_COPY_AND_ASSIGN(Benchmark);
};

// ------------------------------------------------------
// Benchmarks reporter interface + data containers.

// Interface for custom benchmark result printers.
// By default, benchmark reports are printed to stdout. However an application
// can control the destination of the reports by calling
// RunMatchingBenchmarks and passing it a custom reporter object.
// The reporter object must implement the following interface.
class BenchmarkReporter {
 public:
  struct Context {
    int num_cpus;
    double mhz_per_cpu;
    bool cpu_scaling_enabled;

    // The number of chars in the longest benchmark name.
    int name_field_width;
  };

  struct Run {
    Run() :
      iters(1),
      real_accumulated_time(0),
      cpu_accumulated_time(0),
      bytes_per_second(0),
      items_per_second(0),
      max_heapbytes_used(0) {}

    std::string benchmark_name;
    std::string report_label;  // Empty if not set by benchmark.
    int64_t iters;
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
  virtual bool ReportContext(const Context& context) = 0;

  // Called once for each group of benchmark runs, gives information about
  // cpu-time and heap memory usage during the benchmark run.
  // Note that all the grouped benchmark runs should refer to the same
  // benchmark, thus have the same name.
  virtual void ReportRuns(const std::vector<Run>& report) const = 0;

  virtual ~BenchmarkReporter();
};

// Run all benchmarks whose name is a partial match for the regular
// expression in "spec". The results of benchmark runs are fed to "reporter".
void RunMatchingBenchmarks(const std::string& spec,
                           BenchmarkReporter* reporter);

// Extract the list of benchmark names that match the specified regular
// expression.
void FindMatchingBenchmarkNames(const std::string& re,
                                std::vector<std::string>* benchmark_names);

// Simple reporter that outputs benchmark data to the console. This is the
// default reporter used by RunSpecifiedBenchmarks().
class ConsoleReporter : public BenchmarkReporter {
 public:
  virtual bool ReportContext(const Context& context);
  virtual void ReportRuns(const std::vector<Run>& reports) const;
 private:
  virtual void PrintRunData(const Run& report) const;
  // TODO(ericwf): Find a better way to share this information.
  int name_field_width_;
};

}  // end namespace benchmark
#endif  // BENCHMARK_BENCHMARK_H_
