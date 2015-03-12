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

DEFINE_string(benchmark_filter, ".",
              "A regular expression that specifies the set of benchmarks "
              "to execute.  If this flag is empty, no benchmarks are run.  "
              "If this flag is the string \"all\", all benchmarks linked "
              "into the process are run.");

DEFINE_int32(benchmark_iterations, 0,
             "Total number of iterations per benchmark. 0 means the benchmarks "
             "are time-based.");

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

DEFINE_bool(color_print, true, "Enables colorized logging.");

DEFINE_int32(v, 0, "The level of verbose logging to output");


// The ""'s catch people who don't pass in a literal for "str"
#define strliterallen(str) (sizeof("" str "") - 1)

// Must use a string literal for prefix.
#define memprefix(str, len, prefix)                       \
  ((((len) >= strliterallen(prefix)) &&                   \
    std::memcmp(str, prefix, strliterallen(prefix)) == 0) \
       ? str + strliterallen(prefix)                      \
       : nullptr)


namespace benchmark {

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

namespace {

// For non-dense Range, intermediate values are powers of kRangeMultiplier.
static const int kRangeMultiplier = 8;
static const int kMaxIterations = 1000000000;

bool running_benchmark = false;

// Global variable so that a benchmark can cause a little extra printing
std::string* GetReportLabel() {
    static std::string label GUARDED_BY(GetBenchmarkLock());
    return &label;
}

// Should this benchmark base decisions off of real time rather than
// cpu time?
bool use_real_time GUARDED_BY(GetBenchmarkLock());

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
    if (memprefix(buff, bytes_read, "performance") == nullptr) return true;
  }
  return false;
}

void ComputeStats(const std::vector<BenchmarkReporter::Run>& reports,
                  BenchmarkReporter::Run* mean_data,
                  BenchmarkReporter::Run* stddev_data) {
  CHECK(reports.size() >= 2) << "Cannot compute stats for less than 2 reports";
  // Accumulators.
  Stat1_d real_accumulated_time_stat;
  Stat1_d cpu_accumulated_time_stat;
  Stat1_d bytes_per_second_stat;
  Stat1_d items_per_second_stat;
  // All repetitions should be run with the same number of iterations so we
  // can take this information from the first benchmark.
  std::size_t const run_iterations = reports.front().iterations;

  // Populate the accumulators.
  for (BenchmarkReporter::Run const& run : reports) {
    CHECK_EQ(reports[0].benchmark_name, run.benchmark_name);
    CHECK_EQ(run_iterations, run.iterations);
    real_accumulated_time_stat +=
        Stat1_d(run.real_accumulated_time/run.iterations, run.iterations);
    cpu_accumulated_time_stat +=
        Stat1_d(run.cpu_accumulated_time/run.iterations, run.iterations);
    items_per_second_stat += Stat1_d(run.items_per_second, run.iterations);
    bytes_per_second_stat += Stat1_d(run.bytes_per_second, run.iterations);
  }

  // Get the data from the accumulator to BenchmarkReporter::Run's.
  mean_data->benchmark_name = reports[0].benchmark_name + "_mean";
  mean_data->iterations = run_iterations;
  mean_data->real_accumulated_time = real_accumulated_time_stat.Mean() *
                                     run_iterations;
  mean_data->cpu_accumulated_time = cpu_accumulated_time_stat.Mean() *
                                    run_iterations;
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
  stddev_data->iterations = 0;
  stddev_data->real_accumulated_time =
      real_accumulated_time_stat.StdDev();
  stddev_data->cpu_accumulated_time =
      cpu_accumulated_time_stat.StdDev();
  stddev_data->bytes_per_second = bytes_per_second_stat.StdDev();
  stddev_data->items_per_second = items_per_second_stat.StdDev();
}

struct ThreadStats {
    ThreadStats() : bytes_processed(0), items_processed(0) {}
    int64_t bytes_processed;
    int64_t items_processed;
};

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
      CHECK(!running_) <<
        "The timer should be stopped before the timer is finalized";
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
static std::unique_ptr<TimerManager> timer_manager = nullptr;

} // end namespace

namespace internal {

class BenchmarkFamilies;

// Information kept per benchmark we may want to run
struct Benchmark::Instance {
  std::string   name;
  Function*     function;
  bool          has_arg1;
  int           arg1;
  bool          has_arg2;
  int           arg2;
  int           threads;    // Number of concurrent threads to use
  bool          multithreaded;  // Is benchmark multi-threaded?
};


class BenchmarkImp {
public:
  BenchmarkImp(const char* name, Function* func);
  ~BenchmarkImp();

  void Arg(int x);
  void Range(int start, int limit);
  void DenseRange(int start, int limit);
  void ArgPair(int start, int limit);
  void RangePair(int lo1, int hi1, int lo2, int hi2);
  void Threads(int t);
  void ThreadRange(int min_threads, int max_threads);
  void ThreadPerCpu();

  static void AddRange(std::vector<int>* dst, int lo, int hi, int mult);

private:
  friend class BenchmarkFamilies;

  std::string name_;
  Function* function_;
  int arg_count_;
  std::vector< std::pair<int, int> > args_;  // Args for all benchmark runs
  std::vector<int> thread_counts_;
  std::size_t registration_index_;
};

// Class for managing registered benchmarks.  Note that each registered
// benchmark identifies a family of related benchmarks to run.
class BenchmarkFamilies {
 public:
  static BenchmarkFamilies* GetInstance();

  // Registers a benchmark family and returns the index assigned to it.
  size_t AddBenchmark(BenchmarkImp* family);

  // Unregisters a family at the given index.
  void RemoveBenchmark(size_t index);

  // Extract the list of benchmark instances that match the specified
  // regular expression.
  bool FindBenchmarks(const std::string& re,
                      std::vector<Benchmark::Instance>* benchmarks);
 private:
  BenchmarkFamilies();
  ~BenchmarkFamilies();

  std::vector<BenchmarkImp*> families_;
  Mutex mutex_;
};


BenchmarkFamilies* BenchmarkFamilies::GetInstance() {
  static BenchmarkFamilies instance;
  return &instance;
}

BenchmarkFamilies::BenchmarkFamilies() { }

BenchmarkFamilies::~BenchmarkFamilies() {
  for (BenchmarkImp* family : families_) {
    delete family;
  }
}

size_t BenchmarkFamilies::AddBenchmark(BenchmarkImp* family) {
  MutexLock l(mutex_);
  // This loop attempts to reuse an entry that was previously removed to avoid
  // unncessary growth of the vector.
  for (size_t index = 0; index < families_.size(); ++index) {
    if (families_[index] == nullptr) {
      families_[index] = family;
      return index;
    }
  }
  size_t index = families_.size();
  families_.push_back(family);
  return index;
}

void BenchmarkFamilies::RemoveBenchmark(size_t index) {
  MutexLock l(mutex_);
  families_[index] = nullptr;
  // Don't shrink families_ here, we might be called by the destructor of
  // BenchmarkFamilies which iterates over the vector.
}

bool BenchmarkFamilies::FindBenchmarks(
    const std::string& spec,
    std::vector<Benchmark::Instance>* benchmarks) {
  // Make regular expression out of command-line flag
  std::string error_msg;
  Regex re;
  if (!re.Init(spec, &error_msg)) {
    std::cerr << "Could not compile benchmark re: " << error_msg << std::endl;
    return false;
  }

  // Special list of thread counts to use when none are specified
  std::vector<int> one_thread;
  one_thread.push_back(1);

  MutexLock l(mutex_);
  for (BenchmarkImp* family : families_) {
    // Family was deleted or benchmark doesn't match
    if (family == nullptr || !re.Match(family->name_)) continue;

    if (family->arg_count_ == -1) {
      family->arg_count_ = 0;
      family->args_.emplace_back(-1, -1);
    }
    for (auto const& args : family->args_) {
      const std::vector<int>* thread_counts =
        (family->thread_counts_.empty()
         ? &one_thread
         : &family->thread_counts_);
      for (int num_threads : *thread_counts) {

        Benchmark::Instance instance;
        instance.name = family->name_;
        instance.function = family->function_;
        instance.has_arg1 = family->arg_count_ >= 1;
        instance.arg1 = args.first;
        instance.has_arg2 = family->arg_count_ == 2;
        instance.arg2 = args.second;
        instance.threads = num_threads;
        instance.multithreaded = !(family->thread_counts_.empty());

        // Add arguments to instance name
        if (family->arg_count_ >= 1) {
          AppendHumanReadable(instance.arg1, &instance.name);
        }
        if (family->arg_count_ >= 2) {
          AppendHumanReadable(instance.arg2, &instance.name);
        }

        // Add the number of threads used to the name
        if (!family->thread_counts_.empty()) {
          instance.name += StringPrintF("/threads:%d", instance.threads);
        }

        benchmarks->push_back(instance);
      }
    }
  }
  return true;
}

BenchmarkImp::BenchmarkImp(const char* name, Function* func)
    : name_(name), function_(func), arg_count_(-1) {
    registration_index_ = BenchmarkFamilies::GetInstance()->AddBenchmark(this);
}

BenchmarkImp::~BenchmarkImp() {
  BenchmarkFamilies::GetInstance()->RemoveBenchmark(registration_index_);
}

void BenchmarkImp::Arg(int x) {
  CHECK(arg_count_ == -1 || arg_count_ == 1);
  arg_count_ = 1;
  args_.emplace_back(x, -1);
}

void BenchmarkImp::Range(int start, int limit) {
  CHECK(arg_count_ == -1 || arg_count_ == 1);
  arg_count_ = 1;
  std::vector<int> arglist;
  AddRange(&arglist, start, limit, kRangeMultiplier);

  for (int i : arglist) {
    args_.emplace_back(i, -1);
  }
}

void BenchmarkImp::DenseRange(int start, int limit) {
  CHECK(arg_count_ == -1 || arg_count_ == 1);
  arg_count_ = 1;
  CHECK_GE(start, 0);
  CHECK_LE(start, limit);
  for (int arg = start; arg <= limit; arg++) {
    args_.emplace_back(arg, -1);
  }
}

void BenchmarkImp::ArgPair(int x, int y) {
  CHECK(arg_count_ == -1 || arg_count_ == 2);
  arg_count_ = 2;
  args_.emplace_back(x, y);
}

void BenchmarkImp::RangePair(int lo1, int hi1, int lo2, int hi2) {
  CHECK(arg_count_ == -1 || arg_count_ == 2);
  arg_count_ = 2;
  std::vector<int> arglist1, arglist2;
  AddRange(&arglist1, lo1, hi1, kRangeMultiplier);
  AddRange(&arglist2, lo2, hi2, kRangeMultiplier);

  for (int i : arglist1) {
    for (int j : arglist2) {
      args_.emplace_back(i, j);
    }
  }
}

void BenchmarkImp::Threads(int t) {
  CHECK_GT(t, 0);
  thread_counts_.push_back(t);
}

void BenchmarkImp::ThreadRange(int min_threads, int max_threads) {
  CHECK_GT(min_threads, 0);
  CHECK_GE(max_threads, min_threads);

  AddRange(&thread_counts_, min_threads, max_threads, 2);
}

void BenchmarkImp::ThreadPerCpu() {
  static int num_cpus = NumCPUs();
  thread_counts_.push_back(num_cpus);
}

void BenchmarkImp::AddRange(std::vector<int>* dst, int lo, int hi, int mult) {
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

Benchmark::Benchmark(const char* name, Function* f)
    : imp_(new BenchmarkImp(name, f))
{
}

Benchmark::~Benchmark()  {
  delete imp_;
}

Benchmark* Benchmark::Arg(int x) {
  imp_->Arg(x);
  return this;
}

Benchmark* Benchmark::Range(int start, int limit) {
  imp_->Range(start, limit);
  return this;
}

Benchmark* Benchmark::DenseRange(int start, int limit) {
  imp_->DenseRange(start, limit);
  return this;
}

Benchmark* Benchmark::ArgPair(int x, int y) {
  imp_->ArgPair(x, y);
  return this;
}

Benchmark* Benchmark::RangePair(int lo1, int hi1, int lo2, int hi2) {
  imp_->RangePair(lo1, hi1, lo2, hi2);
  return this;
}

Benchmark* Benchmark::Apply(void (*custom_arguments)(Benchmark* benchmark)) {
  custom_arguments(this);
  return this;
}

Benchmark* Benchmark::Threads(int t) {
  imp_->Threads(t);
  return this;
}

Benchmark* Benchmark::ThreadRange(int min_threads, int max_threads) {
  imp_->ThreadRange(min_threads, max_threads);
  return this;
}

Benchmark* Benchmark::ThreadPerCpu() {
  imp_->ThreadPerCpu();
  return this;
}

} // end namespace internal

namespace {


// Execute one thread of benchmark b for the specified number of iterations.
// Adds the stats collected for the thread into *total.
void RunInThread(const benchmark::internal::Benchmark::Instance* b,
                 int iters, int thread_id,
                 ThreadStats* total) EXCLUDES(GetBenchmarkLock()) {
  State st(iters, b->has_arg1, b->arg1, b->has_arg2, b->arg2, thread_id);
  b->function(st);
  CHECK(st.iterations() == st.max_iterations) <<
    "Benchmark returned before State::KeepRunning() returned false!";
  {
    MutexLock l(GetBenchmarkLock());
    total->bytes_processed += st.bytes_processed();
    total->items_processed += st.items_processed();
  }

  timer_manager->Finalize();
}

void RunBenchmark(const benchmark::internal::Benchmark::Instance& b,
                  const BenchmarkReporter* br) EXCLUDES(GetBenchmarkLock()) {
  int iters = FLAGS_benchmark_iterations ? FLAGS_benchmark_iterations
                                         : 1;
  std::vector<BenchmarkReporter::Run> reports;

  std::vector<std::thread> pool;
  if (b.multithreaded)
    pool.resize(b.threads);

  for (int i = 0; i < FLAGS_benchmark_repetitions; i++) {
    std::string mem;
    while (true) {
      // Try benchmark
      VLOG(2) << "Running " << b.name << " for " << iters << "\n";

      {
        MutexLock l(GetBenchmarkLock());
        GetReportLabel()->clear();
        use_real_time = false;
      }

      Notification done;
      timer_manager = std::unique_ptr<TimerManager>(new TimerManager(b.threads, &done));

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
        for (std::size_t ti = 0; ti < pool.size(); ++ti) {
            pool[ti] = std::thread(&RunInThread, &b, iters, ti, &total);
        }
      } else {
        // Run directly in this thread
        RunInThread(&b, iters, 0, &total);
      }
      done.WaitForNotification();
      running_benchmark = false;

      const double cpu_accumulated_time = timer_manager->cpu_time_used();
      const double real_accumulated_time = timer_manager->real_time_used();
      timer_manager.reset();

      VLOG(2) << "Ran in " << cpu_accumulated_time << "/"
              << real_accumulated_time << "\n";

      // Base decisions off of real time if requested by this benchmark.
      double seconds = cpu_accumulated_time;
      std::string label;
      {
        MutexLock l(GetBenchmarkLock());
        label = *GetReportLabel();
        if (use_real_time) {
          seconds = real_accumulated_time;
        }
      }

      // If this was the first run, was elapsed time or cpu time large enough?
      // If this is not the first run, go with the current value of iter.
      if ((i > 0) ||
          (iters == FLAGS_benchmark_iterations) ||
          (iters >= kMaxIterations) ||
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
        BenchmarkReporter::Run report;
        report.benchmark_name = b.name;
        report.report_label = label;
        // Report the total iterations across all threads.
        report.iterations = static_cast<int64_t>(iters) * b.threads;
        report.real_accumulated_time = real_accumulated_time;
        report.cpu_accumulated_time = cpu_accumulated_time;
        report.bytes_per_second = bytes_per_second;
        report.items_per_second = items_per_second;
        reports.push_back(report);
        break;
      }

      // See how much iterations should be increased by
      // Note: Avoid division by zero with max(seconds, 1ns).
      double multiplier = FLAGS_benchmark_min_time * 1.4 / std::max(seconds, 1e-9);
      // If our last run was at least 10% of FLAGS_benchmark_min_time then we
      // use the multiplier directly. Otherwise we use at most 10 times
      // expansion.
      // NOTE: When the last run was at least 10% of the min time the max
      // expansion should be 14x.
      bool is_significant = (seconds / FLAGS_benchmark_min_time) > 0.1;
      multiplier = is_significant ? multiplier : std::min(10.0, multiplier);
      if (multiplier <= 1.0) multiplier = 2.0;
      double next_iters = std::max(multiplier * iters, iters + 1.0);
      if (next_iters > kMaxIterations) {
        next_iters = kMaxIterations;
      }
      VLOG(3) << "Next iters: " << next_iters << ", " << multiplier << "\n";
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

State::State(size_t max_iters, bool has_x, int x, bool has_y, int y,
             int thread_i)
    : started_(false), total_iterations_(0),
      has_range_x_(has_x), range_x_(x),
      has_range_y_(has_y), range_y_(y),
      bytes_processed_(0), items_processed_(0),
      thread_index(thread_i),
      max_iterations(max_iters)
{
    CHECK(max_iterations != 0) << "At least one iteration must be run";
}

void State::PauseTiming() {
  // Add in time accumulated so far
  CHECK(running_benchmark);
  timer_manager->StopTimer();
}

void State::ResumeTiming() {
  CHECK(running_benchmark);
  timer_manager->StartTimer();
}

void State::UseRealTime() {
  MutexLock l(GetBenchmarkLock());
  use_real_time = true;
}

void State::SetLabel(const char* label) {
  CHECK(running_benchmark);
  MutexLock l(GetBenchmarkLock());
  *GetReportLabel() = label;
}

BenchmarkReporter::~BenchmarkReporter() {}

namespace internal {

bool ConsoleReporter::ReportContext(const Context& context) const {
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

#ifndef NDEBUG
  fprintf(stdout, "Build Type: DEBUG\n");
#endif

  int output_width =
      fprintf(stdout,
              "%-*s %10s %10s %10s\n",
              static_cast<int>(name_field_width_),
              "Benchmark",
              "Time(ns)", "CPU(ns)",
              "Iterations");
  fprintf(stdout, "%s\n", std::string(output_width - 1, '-').c_str());

  return true;
}

void ConsoleReporter::ReportRuns(
    const std::vector<Run>& reports) const {
  if (reports.empty()) {
    return;
  }

  for (Run const& run : reports) {
    CHECK_EQ(reports[0].benchmark_name, run.benchmark_name);
    PrintRunData(run);
  }

  if (reports.size() < 2) {
    // We don't report aggregated data if there was a single run.
    return;
  }

  Run mean_data;
  Run stddev_data;
  ComputeStats(reports, &mean_data, &stddev_data);

  // Output using PrintRun.
  PrintRunData(mean_data);
  PrintRunData(stddev_data);
  fprintf(stdout, "\n");
}

void ConsoleReporter::PrintRunData(const Run& result) const {
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

  double const multiplier = 1e9; // nano second multiplier
  ColorPrintf(COLOR_GREEN, "%-*s ",
              name_field_width_, result.benchmark_name.c_str());
  if (result.iterations == 0) {
    ColorPrintf(COLOR_YELLOW, "%10.0f %10.0f ",
                result.real_accumulated_time * multiplier,
                result.cpu_accumulated_time * multiplier);
  } else {
    ColorPrintf(COLOR_YELLOW, "%10.0f %10.0f ",
                (result.real_accumulated_time * multiplier) /
                    (static_cast<double>(result.iterations)),
                (result.cpu_accumulated_time * multiplier) /
                    (static_cast<double>(result.iterations)));
  }
  ColorPrintf(COLOR_CYAN, "%10lld", result.iterations);
  ColorPrintf(COLOR_DEFAULT, "%*s %*s %s\n",
              13, rate.c_str(),
              18, items.c_str(),
              result.report_label.c_str());
}

void RunMatchingBenchmarks(const std::string& spec,
                           const BenchmarkReporter* reporter) {
  CHECK(reporter != nullptr);
  if (spec.empty()) return;

  std::vector<benchmark::internal::Benchmark::Instance> benchmarks;
  auto families = benchmark::internal::BenchmarkFamilies::GetInstance();
  if (!families->FindBenchmarks(spec, &benchmarks)) return;


  // Determine the width of the name field using a minimum width of 10.
  // Also determine max number of threads needed.
  size_t name_field_width = 10;
  for (const internal::Benchmark::Instance& benchmark : benchmarks) {
    // Add width for _stddev and threads:XX
    if (benchmark.threads > 1 && FLAGS_benchmark_repetitions > 1) {
      name_field_width =
          std::max<size_t>(name_field_width, benchmark.name.size() + 17);
    } else if (benchmark.threads > 1) {
      name_field_width =
          std::max<size_t>(name_field_width, benchmark.name.size() + 10);
    } else if (FLAGS_benchmark_repetitions > 1) {
      name_field_width =
          std::max<size_t>(name_field_width, benchmark.name.size() + 7);
    } else {
      name_field_width =
          std::max<size_t>(name_field_width, benchmark.name.size());
    }
  }

  // Print header here
  BenchmarkReporter::Context context;
  context.num_cpus = NumCPUs();
  context.mhz_per_cpu = CyclesPerSecond() / 1000000.0f;

  context.cpu_scaling_enabled = CpuScalingEnabled();
  context.name_field_width = name_field_width;

  if (reporter->ReportContext(context)) {
    for (const auto& benchmark : benchmarks) {
      RunBenchmark(benchmark, reporter);
    }
  }
}

} // end namespace internal


void RunSpecifiedBenchmarks(const BenchmarkReporter* reporter) {
  std::string spec = FLAGS_benchmark_filter;
  if (spec.empty() || spec == "all")
    spec = ".";  // Regexp that matches all benchmarks
  internal::ConsoleReporter default_reporter;
  internal::RunMatchingBenchmarks(spec, reporter ? reporter : &default_reporter);
}

namespace internal {

void PrintUsageAndExit() {
  fprintf(stdout,
          "benchmark"
          " [--benchmark_filter=<regex>]\n"
          "          [--benchmark_iterations=<iterations>]\n"
          "          [--benchmark_min_time=<min_time>]\n"
          "          [--benchmark_repetitions=<num_repetitions>]\n"
          "          [--color_print={true|false}]\n"
          "          [--v=<verbosity>]\n");
  exit(0);
}

void ParseCommandLineFlags(int* argc, const char** argv) {
  using namespace benchmark;
  for (int i = 1; i < *argc; ++i) {
    if (
        ParseStringFlag(argv[i], "benchmark_filter",
                        &FLAGS_benchmark_filter) ||
        ParseInt32Flag(argv[i], "benchmark_iterations",
                       &FLAGS_benchmark_iterations) ||
        ParseDoubleFlag(argv[i], "benchmark_min_time",
                        &FLAGS_benchmark_min_time) ||
        ParseInt32Flag(argv[i], "benchmark_repetitions",
                       &FLAGS_benchmark_repetitions) ||
        ParseBoolFlag(argv[i], "color_print",
                       &FLAGS_color_print) ||
        ParseInt32Flag(argv[i], "v", &FLAGS_v)) {
      for (int j = i; j != *argc; ++j) argv[j] = argv[j + 1];

      --(*argc);
      --i;
    } else if (IsFlag(argv[i], "help")) {
      PrintUsageAndExit();
    }
  }
}

} // end namespace internal

void Initialize(int* argc, const char** argv) {
  internal::ParseCommandLineFlags(argc, argv);
  internal::SetLogLevel(FLAGS_v);
  // TODO remove this. It prints some output the first time it is called.
  // We don't want to have this ouput printed during benchmarking.
  MyCPUUsage();
  // The first call to walltime::Now initialized it. Call it once to
  // prevent the initialization from happening in a benchmark.
  walltime::Now();
}

} // end namespace benchmark
