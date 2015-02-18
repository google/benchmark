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

#include "benchmark/benchmark.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <thread>

#include "check.h"
#include "commandlineflags.h"
#include "colorprint.h"
#include "log.h"
#include "mutex.h"
#include "re.h"
#include "stat.h"
#include "string_util.h"
#include "sysinfo.h"
#include "walltime.h"


DEFINE_string(benchmarks, "all",
              "A regular expression that specifies the set of benchmarks "
              "to execute.  If this flag is empty, no benchmarks are run.  "
              "If this flag is the string \"all\", all benchmarks linked "
              "into the process are run.");

DEFINE_int32(benchmark_min_iters, 100,
             "Minimum number of iterations per benchmark");

DEFINE_int32(benchmark_max_iters, 1000000000,
             "Maximum number of iterations per benchmark");

DEFINE_double(benchmark_min_time, 0.5,
              "Minimum number of seconds we should run benchmark before "
              "results are considered significant.  For cpu-time based "
              "tests, this is the lower bound on the total cpu time "
              "used by all threads that make up the test.  For real-time "
              "based tests, this is the lower bound on the elapsed time "
              "of the benchmark execution, regardless of number of "
              "threads.");

DEFINE_int32(benchmark_repetitions, 1,
             "The number of runs of each benchmark. If greater than 1, the "
             "mean and standard deviation of the runs will be reported.");

DEFINE_bool(benchmark_use_picoseconds, false,
            "Report times in picoseconds, to provide more significant figures "
            "for very fast-running benchmarks.");

DEFINE_bool(color_print, false, "Enables colorized logging.");

// The ""'s catch people who don't pass in a literal for "str"
#define strliterallen(str) (sizeof("" str "") - 1)

// Must use a string literal for prefix.
#define memprefix(str, len, prefix)                       \
  ((((len) >= strliterallen(prefix)) &&                   \
    std::memcmp(str, prefix, strliterallen(prefix)) == 0) \
       ? str + strliterallen(prefix)                      \
       : NULL)


namespace benchmark {

// For non-dense Range, intermediate values are powers of kRangeMultiplier.
static const int kRangeMultiplier = 8;

namespace internal {

// NOTE: This is a dummy "mutex" type used to denote the actual mutex
// returned by GetBenchmarkLock(). This is only used to placate the thread
// safety warnings by giving the return of GetBenchmarkLock() a name.
struct CAPABILITY("mutex") BenchmarkLockType {};
BenchmarkLockType BenchmarkLockVar;

} // end namespace internal

inline Mutex& RETURN_CAPABILITY(::benchmark::internal::BenchmarkLockVar)
GetBenchmarkLock()
{
  static Mutex lock;
  return lock;
}

// List of all registered benchmarks.  Note that each registered
// benchmark identifies a family of related benchmarks to run.
static std::vector<Benchmark*>* families = NULL;

struct ThreadStats {
  int64_t bytes_processed;
  int64_t items_processed;

  ThreadStats() {
    Reset();
  }
  void Reset() {
    bytes_processed = 0;
    items_processed = 0;
  }
  void Add(const ThreadStats& other) {
    bytes_processed += other.bytes_processed;
    items_processed += other.items_processed;
  }
};

// Per-thread stats
static thread_local ThreadStats thread_stats;

// Timer management class
class TimerManager {
 public:
  TimerManager(int num_threads, Notification* done)
      : num_threads_(num_threads),
        done_(done),
        running_(false),
        real_time_used_(0),
        cpu_time_used_(0),
        num_finalized_(0),
        phase_number_(0),
        entered_(0) {
  }

  // Called by each thread
  void StartTimer() EXCLUDES(lock_) {
    bool last_thread = false;
    {
      MutexLock ml(lock_);
      last_thread = Barrier(ml);
      if (last_thread) {
        CHECK(!running_) << "Called StartTimer when timer is already running";
        running_ = true;
        start_real_time_ = walltime::Now();
        start_cpu_time_ = MyCPUUsage() + ChildrenCPUUsage();
       }
     }
     if (last_thread) {
       phase_condition_.notify_all();
     }
  }

  // Called by each thread
  void StopTimer() EXCLUDES(lock_) {
    bool last_thread = false;
    {
      MutexLock ml(lock_);
      last_thread = Barrier(ml);
      if (last_thread) {
        CHECK(running_) << "Called StopTimer when timer is already stopped";
        InternalStop();
      }
    }
    if (last_thread) {
      phase_condition_.notify_all();
    }
  }

  // Called by each thread
  void Finalize() EXCLUDES(lock_) {
    MutexLock l(lock_);
    num_finalized_++;
    if (num_finalized_ == num_threads_) {
      if (running_) {
        InternalStop();
      }
      done_->Notify();
    }
  }

  // REQUIRES: timer is not running
  double real_time_used() EXCLUDES(lock_) {
    MutexLock l(lock_);
    CHECK(!running_);
    return real_time_used_;
  }

  // REQUIRES: timer is not running
  double cpu_time_used() EXCLUDES(lock_) {
    MutexLock l(lock_);
    CHECK(!running_);
    return cpu_time_used_;
  }

 private:
  Mutex lock_;
  Condition phase_condition_;
  int num_threads_;
  Notification* done_;

  bool running_;                // Is the timer running
  double start_real_time_;      // If running_
  double start_cpu_time_;       // If running_

  // Accumulated time so far (does not contain current slice if running_)
  double real_time_used_;
  double cpu_time_used_;

  // How many threads have called Finalize()
  int num_finalized_;

  // State for barrier management
  int phase_number_;
  int entered_;         // Number of threads that have entered this barrier

  void InternalStop() REQUIRES(lock_) {
    CHECK(running_);
    running_ = false;
    real_time_used_ += walltime::Now() - start_real_time_;
    cpu_time_used_ += ((MyCPUUsage() + ChildrenCPUUsage())
                       - start_cpu_time_);
  }

  // Enter the barrier and wait until all other threads have also
  // entered the barrier.  Returns iff this is the last thread to
  // enter the barrier.
  bool Barrier(MutexLock& ml) REQUIRES(lock_) {
    CHECK_LT(entered_, num_threads_);
    entered_++;
    if (entered_ < num_threads_) {
      // Wait for all threads to enter
      int phase_number_cp = phase_number_;
      auto cb = [this, phase_number_cp]() {
        return this->phase_number_ > phase_number_cp;
      };
      phase_condition_.wait(ml.native_handle(), cb);
      return false;  // I was not the last one
    } else {
      // Last thread has reached the barrier
      phase_number_++;
      entered_ = 0;
      return true;
    }
  }
};

// TimerManager for current run.
static TimerManager* timer_manager = nullptr;

const int Benchmark::kNumCpuMarker;

// Information kept per benchmark we may want to run
struct Benchmark::Instance {
  std::string   name;
  Function      function;
  Function      setup;
  Function      teardown;
  int           arg1;
  int           arg2;
  int           threads;    // Number of concurrent threads to use
  bool          multithreaded;  // Is benchmark multi-threaded?

  void Run(int iters) const {
    function.Run(iters, arg1, arg2);
  }
};

void Function::Run(int iters, int arg1, int arg2) const {
  if (f0_ != NULL) {
    (*f0_)(iters);
  } else if (f1_ != NULL) {
    (*f1_)(iters, arg1);
  } else if (f2_ != NULL) {
    (*f2_)(iters, arg1, arg2);
  } else {
    // NULL function; do nothing.
  }
}

int Function::args() const {
  if (f0_ != NULL) {
    return 0;
  } else if (f1_ != NULL) {
    return 1;
  } else if (f2_ != NULL) {
    return 2;
  } else {
    return -1;
  }
}

Benchmark::Benchmark(const std::string& name,
                     const Function& f) EXCLUDES(GetBenchmarkLock())
                    : name_(name), function_(f) {
  MutexLock l(GetBenchmarkLock());
  if (families == NULL) {
    families = new std::vector<Benchmark*>;
  }
  registration_index_ = families->size();
  families->push_back(this);
  if (f.args() == 0) {
    // Run it exactly once regardless of Arg/Range calls.
    args_.emplace_back(-1, -1);
  }
}

Benchmark::~Benchmark() EXCLUDES(GetBenchmarkLock()) {
  MutexLock l(GetBenchmarkLock());
  CHECK((*families)[registration_index_] == this);
  (*families)[registration_index_] = NULL;
  // Shrink the vector if convenient.
  while (!families->empty() && families->back() == NULL) {
    families->pop_back();
  }
}

Benchmark* Benchmark::Arg(int x) {
  CHECK_EQ(function_.args(), 1) << "Wrong number of args for " << name_;
  args_.emplace_back(x, -1);
  return this;
}

Benchmark* Benchmark::Range(int start, int limit) {
  CHECK_EQ(function_.args(), 1) << "Wrong number of args for " << name_;
  std::vector<int> arglist;
  AddRange(&arglist, start, limit, kRangeMultiplier);

  for (int i : arglist) {
    args_.emplace_back(i, -1);
  }
  return this;
}

Benchmark* Benchmark::DenseRange(int start, int limit) {
  CHECK_EQ(function_.args(), 1) << "Wrong number of args for " << name_;
  CHECK_GE(start, 0);
  CHECK_LE(start, limit);
  for (int arg = start; arg <= limit; arg++) {
    args_.emplace_back(arg, -1);
  }
  return this;
}

Benchmark* Benchmark::ArgPair(int x, int y) {
  CHECK_EQ(function_.args(), 2) << "Wrong number of args for " << name_;
  args_.emplace_back(x, y);
  return this;
}

Benchmark* Benchmark::RangePair(int lo1, int hi1, int lo2, int hi2) {
  CHECK_EQ(function_.args(), 2) << "Wrong number of args for " << name_;
  std::vector<int> arglist1, arglist2;
  AddRange(&arglist1, lo1, hi1, kRangeMultiplier);
  AddRange(&arglist2, lo2, hi2, kRangeMultiplier);

  for (int i : arglist1) {
    for (int j : arglist2) {
      args_.emplace_back(i, j);
    }
  }
  return this;
}

Benchmark* Benchmark::Apply(void (*custom_arguments)(Benchmark* benchmark)) {
  custom_arguments(this);
  return this;
}

Benchmark* Benchmark::Threads(int t) {
  CHECK_GT(t, 0);
  thread_counts_.push_back(t);
  return this;
}

Benchmark* Benchmark::ThreadRange(int min_threads, int max_threads) {
  CHECK_GT(min_threads, 0);
  CHECK_GE(max_threads, min_threads);

  AddRange(&thread_counts_, min_threads, max_threads, 2);
  return this;
}

Benchmark* Benchmark::ThreadPerCpu() {
  thread_counts_.push_back(kNumCpuMarker);
  return this;
}

void Benchmark::AddRange(std::vector<int>* dst, int lo, int hi, int mult) {
  CHECK_GE(lo, 0);
  CHECK_GE(hi, lo);

  // Add "lo"
  dst->push_back(lo);

  static const int kint32max = std::numeric_limits<int32_t>::max();

  // Now space out the benchmarks in multiples of "mult"
  for (int32_t i = 1; i < kint32max/mult; i *= mult) {
    if (i >= hi) break;
    if (i > lo) {
      dst->push_back(i);
    }
  }
  // Add "hi" (if different from "lo")
  if (hi != lo) {
    dst->push_back(hi);
  }
}

Benchmark* Benchmark::Setup(const Function& setup) {
  setup_ = setup;
  return this;
}

Benchmark* Benchmark::Teardown(const Function& teardown) {
  teardown_ = teardown;
  return this;
}

// Extract the list of benchmark instances that match the specified
// regular expression.
void Benchmark::FindBenchmarks(
    const std::string& spec,
    std::vector<Instance>* benchmarks) EXCLUDES(GetBenchmarkLock()) {
  // Make regular expression out of command-line flag
  std::string error_msg;
  Regex re;
  if (!re.Init(spec, &error_msg)) {
    std::cerr << "Could not compile benchmark re: " << error_msg << std::endl;
    std::exit(1);
  }

  // Special list of thread counts to use when none are specified
  std::vector<int> one_thread;
  one_thread.push_back(1);

  MutexLock l(GetBenchmarkLock());
  if (families != NULL) {
    for (Benchmark* family : *families) {
      if (family == NULL) continue;  // Family was deleted

      const int num_args = family->function_.args();
      for (auto const& args : family->args_) {
        const std::vector<int>* thread_counts =
            (family->thread_counts_.empty()
             ? &one_thread
             : &family->thread_counts_);
        for (int num_threads : *thread_counts) {
          if (num_threads == kNumCpuMarker) {
            num_threads = benchmark::NumCPUs();
          }

          Instance instance;
          instance.name = family->name_;
          instance.function = family->function_;
          instance.arg1 = args.first;
          instance.arg2 = args.second;
          instance.threads = num_threads;
          instance.multithreaded = !(family->thread_counts_.empty());
          instance.setup = family->setup_;
          instance.teardown = family->teardown_;

          // Add arguments to instance name
          if (num_args >= 1) {
            AppendHumanReadable(instance.arg1, &instance.name);
          }
          if (num_args >= 2) {
            AppendHumanReadable(instance.arg2, &instance.name);
          }

          // Add the number of threads used to the name
          if (!family->thread_counts_.empty()) {
            instance.name += StringPrintF("/threads:%d", instance.threads);
          }

          // Add if benchmark name matches regexp
          if (re.Match(instance.name)) {
            benchmarks->push_back(instance);
          }
        }
      }
    }
  }
}


namespace {

bool running_benchmark = false;

// Global variable so that a benchmark can cause a little extra printing
std::string report_label GUARDED_BY(GetBenchmarkLock());

// Should this benchmark base decisions off of real time rather than
// cpu time?
bool use_real_time GUARDED_BY(GetBenchmarkLock());

// Return prefix to print in front of each reported line
static const char* Prefix() {
#ifdef NDEBUG
  return "";
#else
  return "DEBUG: ";
#endif
}

// TODO(ericwf): support MallocCounter.
//static benchmark::MallocCounter *benchmark_mc;

static bool CpuScalingEnabled() {
  // On Linux, the CPUfreq subsystem exposes CPU information as files on the
  // local file system. If reading the exported files fails, then we may not be
  // running on Linux, so we silently ignore all the read errors.
  for (int cpu = 0, num_cpus = NumCPUs(); cpu < num_cpus; ++cpu) {
    std::string governor_file = StrCat("/sys/devices/system/cpu/cpu", cpu,
                                       "/cpufreq/scaling_governor");
    FILE* file = fopen(governor_file.c_str(), "r");
    if (!file) break;
    char buff[16];
    size_t bytes_read = fread(buff, 1, sizeof(buff), file);
    fclose(file);
    if (memprefix(buff, bytes_read, "performance") == NULL) return true;
  }
  return false;
}

// Execute one thread of benchmark b for the specified number of iterations.
// Adds the stats collected for the thread into *total.
void RunInThread(const benchmark::Benchmark::Instance* b,
                 int iters,
                 ThreadStats* total) EXCLUDES(GetBenchmarkLock()) {
  ThreadStats* my_stats = &thread_stats;
  my_stats->Reset();
  timer_manager->StartTimer();
  b->Run(iters);

  {
    MutexLock l(GetBenchmarkLock());
    total->Add(*my_stats);
  }

  timer_manager->Finalize();
}

void RunBenchmark(const benchmark::Benchmark::Instance& b,
                  BenchmarkReporter* br) EXCLUDES(GetBenchmarkLock()) {
  int iters = FLAGS_benchmark_min_iters;
  std::vector<BenchmarkRunData> reports;

  std::vector<std::thread> pool;
  if (b.multithreaded)
    pool.resize(b.threads);

  for (int i = 0; i < FLAGS_benchmark_repetitions; i++) {
    std::string mem;
    while (true) {
      // Try benchmark
      VLOG(1) << "Running " << b.name << " for " << iters << "\n";

      {
        MutexLock l(GetBenchmarkLock());
        report_label.clear();
        use_real_time = false;
      }
      b.setup.Run(b.threads, b.arg1, b.arg2);

      Notification done;
      timer_manager = new TimerManager(b.threads, &done);

      ThreadStats total;
      running_benchmark = true;
      if (b.multithreaded) {
        // If this is out first iteration of the while(true) loop then the
        // threads haven't been started and can't be joined. Otherwise we need
        // to join the thread before replacing them.
        for (std::thread& thread : pool) {
          if (thread.joinable())
            thread.join();
        }
        for (std::thread& thread : pool) {
          thread = std::thread(&RunInThread, &b, iters, &total);
        }
      } else {
        // Run directly in this thread
        RunInThread(&b, iters, &total);
      }
      done.WaitForNotification();
      running_benchmark = false;

      const double cpu_accumulated_time = timer_manager->cpu_time_used();
      const double real_accumulated_time = timer_manager->real_time_used();
      delete timer_manager;
      timer_manager = NULL;
      b.teardown.Run(b.threads, b.arg1, b.arg2);

      VLOG(1) << "Ran in " << cpu_accumulated_time << "/"
            << real_accumulated_time << "\n";

      // Base decisions off of real time if requested by this benchmark.
      double seconds = cpu_accumulated_time;
      std::string label;
      {
        MutexLock l(GetBenchmarkLock());
        label = report_label;
        if (use_real_time) {
          seconds = real_accumulated_time;
        }
      }

      // If this was the first run, was elapsed time or cpu time large enough?
      // If this is not the first run, go with the current value of iter.
      if ((i > 0) ||
          (iters >= FLAGS_benchmark_max_iters) ||
          (seconds >= FLAGS_benchmark_min_time) ||
          (real_accumulated_time >= 5*FLAGS_benchmark_min_time)) {
        double bytes_per_second = 0;
        if (total.bytes_processed > 0 && seconds != 0.0) {
          bytes_per_second = (total.bytes_processed / seconds);
        }
        double items_per_second = 0;
        if (total.items_processed > 0 && seconds != 0.0) {
          items_per_second = (total.items_processed / seconds);
        }

        // Create report about this benchmark run.
        BenchmarkRunData report;
        report.benchmark_name = b.name;
        report.report_label = label;
        // Report the total iterations across all threads.
        report.iters = static_cast<int64_t>(iters) * b.threads;
        report.real_accumulated_time = real_accumulated_time;
        report.cpu_accumulated_time = cpu_accumulated_time;
        report.bytes_per_second = bytes_per_second;
        report.items_per_second = items_per_second;
        reports.push_back(report);
        break;
      }

      // See how much iterations should be increased by
      // Note: Avoid division by zero with max(seconds, 1ns).
      double multiplier = 1.4 * FLAGS_benchmark_min_time /
                          std::max(seconds, 1e-9);
      multiplier = std::min(10.0, multiplier);  // At most 10 times expansion
      if (multiplier <= 1.0) multiplier = 2.0;
      double next_iters = std::max(multiplier * iters, iters + 1.0);
      if (next_iters > FLAGS_benchmark_max_iters) {
        next_iters = FLAGS_benchmark_max_iters;
      }
      VLOG(2) << "Next iters: " << next_iters << ", " << multiplier << "\n";
      iters = static_cast<int>(next_iters + 0.5);
    }
  }
  br->ReportRuns(reports);
  if (b.multithreaded) {
    for (std::thread& thread : pool)
      thread.join();
  }
}

}  // namespace


BenchmarkReporter::~BenchmarkReporter() {}


void ComputeStats(const std::vector<BenchmarkRunData>& reports,
                  BenchmarkRunData* mean_data,
                  BenchmarkRunData* stddev_data) {
  // Accumulators.
  Stat1_d real_accumulated_time_stat;
  Stat1_d cpu_accumulated_time_stat;
  Stat1_d bytes_per_second_stat;
  Stat1_d items_per_second_stat;
  int64_t total_iters = 0;

  // Populate the accumulators.
  for (std::vector<BenchmarkRunData>::const_iterator it = reports.begin();
       it != reports.end();
       it++) {
    CHECK_EQ(reports[0].benchmark_name, it->benchmark_name);
    total_iters += it->iters;
    real_accumulated_time_stat +=
        Stat1_d(it->real_accumulated_time/it->iters, it->iters);
    cpu_accumulated_time_stat +=
        Stat1_d(it->cpu_accumulated_time/it->iters, it->iters);
    items_per_second_stat += Stat1_d(it->items_per_second, it->iters);
    bytes_per_second_stat += Stat1_d(it->bytes_per_second, it->iters);
  }

  // Get the data from the accumulator to BenchmarkRunData's.
  mean_data->benchmark_name = reports[0].benchmark_name + "_mean";
  mean_data->iters = total_iters;
  mean_data->real_accumulated_time = real_accumulated_time_stat.Sum();
  mean_data->cpu_accumulated_time = cpu_accumulated_time_stat.Sum();
  mean_data->bytes_per_second = bytes_per_second_stat.Mean();
  mean_data->items_per_second = items_per_second_stat.Mean();

  // Only add label to mean/stddev if it is same for all runs
  mean_data->report_label = reports[0].report_label;
  for (std::size_t i = 1; i < reports.size(); i++) {
    if (reports[i].report_label != reports[0].report_label) {
      mean_data->report_label = "";
      break;
    }
  }

  stddev_data->benchmark_name = reports[0].benchmark_name + "_stddev";
  stddev_data->report_label = mean_data->report_label;
  stddev_data->iters = total_iters;
  // We multiply by total_iters since PrintRunData expects a total time.
  stddev_data->real_accumulated_time =
      real_accumulated_time_stat.StdDev() * total_iters;
  stddev_data->cpu_accumulated_time =
      cpu_accumulated_time_stat.StdDev() * total_iters;
  stddev_data->bytes_per_second = bytes_per_second_stat.StdDev();
  stddev_data->items_per_second = items_per_second_stat.StdDev();
}


bool ConsoleReporter::ReportContext(const BenchmarkContextData& context) {
  name_field_width_ = context.name_field_width;

  fprintf(stdout,
          "Run on (%d X %0.0f MHz CPU%s)\n",
          context.num_cpus,
          context.mhz_per_cpu,
          (context.num_cpus > 1) ? "s" : "");

  int remainder_us;
  std::string walltime_str = walltime::Print(
                                walltime::Now(), "%Y/%m/%d-%H:%M:%S",
                                true,  // use local timezone
                                &remainder_us);
  fprintf(stdout, "%s\n", walltime_str.c_str());

  if (context.cpu_scaling_enabled) {
    fprintf(stdout, "***WARNING*** CPU scaling is enabled, the benchmark "
                    "timings may be noisy\n");
  }

  int output_width =
      fprintf(stdout,
              "%s%-*s %10s %10s %10s\n",
              Prefix(),
              name_field_width_,
              "Benchmark",
              FLAGS_benchmark_use_picoseconds ? "Time(ps)" : "Time(ns)",
              FLAGS_benchmark_use_picoseconds ? "CPU(ps)" : "CPU(ns)",
              "Iterations");
  fprintf(stdout, "%s\n", std::string(output_width - 1, '-').c_str());

  return true;
}

void ConsoleReporter::ReportRuns(const std::vector<BenchmarkRunData>& reports) {
  if (reports.empty()) {
    return;
  }

  for (std::vector<BenchmarkRunData>::const_iterator it = reports.begin();
       it != reports.end();
       it++) {
    CHECK_EQ(reports[0].benchmark_name, it->benchmark_name);
    PrintRunData(*it);
  }

  if (reports.size() < 2) {
    // We don't report aggregated data if there was a single run.
    return;
  }

  BenchmarkRunData mean_data;
  BenchmarkRunData stddev_data;
  benchmark::ComputeStats(reports, &mean_data, &stddev_data);

  // Output using PrintRun.
  PrintRunData(mean_data);
  PrintRunData(stddev_data);
  fprintf(stdout, "\n");
}

void ConsoleReporter::PrintRunData(const BenchmarkRunData& result) {
  // Format bytes per second
  std::string rate;
  if (result.bytes_per_second > 0) {
    rate = StrCat(" ", HumanReadableNumber(result.bytes_per_second), "B/s");
  }

  // Format items per second
  std::string items;
  if (result.items_per_second > 0) {
    items = StrCat(" ", HumanReadableNumber(result.items_per_second),
                   " items/s");
  }

  double const multiplier = FLAGS_benchmark_use_picoseconds ? 1e12 : 1e9;
  ColorPrintf(COLOR_DEFAULT, "%s", Prefix());
  ColorPrintf(COLOR_GREEN, "%-*s ",
              name_field_width_, result.benchmark_name.c_str());
  if (result.iters == 0) {
    ColorPrintf(COLOR_YELLOW, "%10.0f %10.0f ",
                result.real_accumulated_time * multiplier,
                result.cpu_accumulated_time * multiplier);
  } else {
    ColorPrintf(COLOR_YELLOW, "%10.0f %10.0f ",
                (result.real_accumulated_time * multiplier) /
                    (static_cast<double>(result.iters)),
                (result.cpu_accumulated_time * multiplier) /
                    (static_cast<double>(result.iters)));
  }
  ColorPrintf(COLOR_CYAN, "%10lld", result.iters);
  ColorPrintf(COLOR_DEFAULT, "%*s %*s %s\n",
              13, rate.c_str(),
              18, items.c_str(),
              result.report_label.c_str());
}

void RunMatchingBenchmarks(const std::string& spec,
                           BenchmarkReporter* reporter) {
  CHECK(reporter != NULL);
  if (spec.empty()) return;

  CHECK(FLAGS_benchmark_min_iters <= FLAGS_benchmark_max_iters)
    << "-benchmark_min_iters=" << FLAGS_benchmark_min_iters
    << " must be less than or equal to -benchmark_max_iters="
    << FLAGS_benchmark_max_iters;

  std::vector<benchmark::Benchmark::Instance> benchmarks;
  benchmark::Benchmark::FindBenchmarks(spec, &benchmarks);


  // Determine the width of the name field using a minimum width of 10.
  int name_field_width = 10;
  for (std::size_t i = 0; i < benchmarks.size(); i++) {
    name_field_width = std::max<int>(name_field_width,
                                    // Maybe add space for appending "_stddev"
                                    FLAGS_benchmark_repetitions > 1
                                    ? benchmarks[i].name.size() + 7
                                    : benchmarks[i].name.size());
  }

  // Print header here
  BenchmarkContextData context;
  context.num_cpus = benchmark::NumCPUs();
  context.mhz_per_cpu = benchmark::CyclesPerSecond() / 1000000.0f;

  context.cpu_scaling_enabled = CpuScalingEnabled();
  context.name_field_width = name_field_width;

  if (reporter->ReportContext(context)) {
    for (const auto& bench_instance : benchmarks) {
      RunBenchmark(bench_instance, reporter);
    }
  }
}

void FindMatchingBenchmarkNames(const std::string& spec,
                                std::vector<std::string>* benchmark_names) {
  if (spec.empty()) return;

  std::vector<benchmark::Benchmark::Instance> benchmarks;
  benchmark::Benchmark::FindBenchmarks(spec, &benchmarks);
  for (const auto& bench_instance : benchmarks) {
    benchmark_names->push_back(bench_instance.name);
  }
}

void RunSpecifiedBenchmarks(BenchmarkReporter* reporter) {
  std::string spec = FLAGS_benchmarks;
  if (spec.empty()) {
    // Nothing to do
  } else {
    if (spec == "all") {
      spec = ".";         // Regexp that matches all benchmarks
    }
    ConsoleReporter default_reporter;
    RunMatchingBenchmarks(spec, reporter ? reporter : &default_reporter);
    std::exit(0);
  }
}

void SetBenchmarkBytesProcessed(int64_t bytes) {
  CHECK(running_benchmark) << ": SetBenchmarkBytesProcessed() should only "
                           << "be called from within a benchmark";
  thread_stats.bytes_processed = bytes;
}

void SetBenchmarkItemsProcessed(int64_t items) {
  CHECK(running_benchmark) << ": SetBenchmarkItemsProcessed() should only "
                           << "be called from within a benchmark";
  thread_stats.items_processed = items;
}

void SetBenchmarkLabel(const char* label) {
  CHECK(running_benchmark);
  MutexLock l(GetBenchmarkLock());
  report_label = label;
}

void StopBenchmarkTiming() {
  // Add in time accumulated so far
  CHECK(running_benchmark);
  timer_manager->StopTimer();
}

void StartBenchmarkTiming() {
  CHECK(running_benchmark);
  timer_manager->StartTimer();
}


void BenchmarkUseRealTime() {
  MutexLock l(GetBenchmarkLock());
  use_real_time = true;
}

void PrintUsageAndExit() {
  fprintf(stdout,
          "benchmark"
          " [--benchmarks=<regex>]\n"

          "          [--benchmark_min_iters=<iterations>]\n"
          "          [--benchmark_max_iters=<iterations>]\n"
          "          [--benchmark_min_time=<min_time>]\n"
          "          [--benchmark_repetitions=<num_repetitions>]\n"
          "          [--benchmark_use_picoseconds={true|false}]\n"
          "          [--color_print={true|false}]\n");
  exit(0);
}

void ParseCommandLineFlags(int* argc, const char** argv) {
  using namespace benchmark;
  for (int i = 1; i < *argc; ++i) {
    if (
        ParseStringFlag(argv[i], "benchmarks", &FLAGS_benchmarks) ||
        ParseInt32Flag(argv[i], "benchmark_min_iters",
                       &FLAGS_benchmark_min_iters) ||
        ParseInt32Flag(argv[i], "benchmark_max_iters",
                       &FLAGS_benchmark_max_iters) ||
        ParseDoubleFlag(argv[i], "benchmark_min_time",
                        &FLAGS_benchmark_min_time) ||
        ParseInt32Flag(argv[i], "benchmark_repetitions",
                       &FLAGS_benchmark_repetitions) ||
        ParseBoolFlag(argv[i], "benchmark_use_picoseconds",
                       &FLAGS_benchmark_use_picoseconds) ||
        ParseBoolFlag(argv[i], "color_print",
                       &FLAGS_color_print)) {
      for (int j = i; j != *argc; ++j) argv[j] = argv[j + 1];

      --(*argc);
      --i;
    } else if (IsFlag(argv[i], "help")) {
      PrintUsageAndExit();
    } else {
        std::cerr << "Unrecognized option: " << argv[i] << "\n";
        PrintUsageAndExit();
    }
  }
}


void Initialize(int argc, const char** argv) {
  ParseCommandLineFlags(&argc, argv);
  // TODO remove this. It prints some output the first time it is called.
  // We don't want to have this ouput printed during benchmarking.
  MyCPUUsage();
  // The first call to walltime::Now initialized it. Call it once to
  // prevent the initialization from happening in a benchmark.
  walltime::Now();
}

} // end namespace benchmark
