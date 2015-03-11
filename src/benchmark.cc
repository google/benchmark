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
#include "arraysize.h"
#include "check.h"
#include "colorprint.h"
#include "commandlineflags.h"
#include "internal_macros.h"
#include "log.h"
#include "re.h"
#include "sleep.h"
#include "stat.h"
#include "string_util.h"
#include "sysinfo.h"
#include "walltime.h"

#include <sys/time.h>
#include <string.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <sstream>

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

DEFINE_bool(benchmark_memory_usage, false,
            "Report memory usage for all benchmarks");

DEFINE_int32(benchmark_repetitions, 1,
             "The number of runs of each benchmark. If greater than 1, the "
             "mean and standard deviation of the runs will be reported.");

DEFINE_int32(v, 0, "The level of verbose logging to output");
DEFINE_bool(color_print, true, "Enables colorized logging.");

// Will be non-empty if heap checking is turned on, which would
// invalidate any benchmarks.
DECLARE_string(heap_check);

// The ""'s catch people who don't pass in a literal for "str"
#define strliterallen(str) (sizeof("" str "") - 1)

// Must use a string literal for prefix.
#define memprefix(str, len, prefix)                  \
  ((((len) >= strliterallen(prefix)) &&              \
    memcmp(str, prefix, strliterallen(prefix)) == 0) \
       ? str + strliterallen(prefix)                 \
       : NULL)

namespace benchmark {
namespace {
// For non-dense Range, intermediate values are powers of kRangeMultiplier.
static const int kRangeMultiplier = 8;

std::mutex starting_mutex;
std::condition_variable starting_cv;

bool running_benchmark = false;

// Should this benchmark report memory usage?
bool get_memory_usage;

// Should this benchmark base decisions off of real time rather than
// cpu time?
bool use_real_time;

// Overhead of an empty benchmark.
double overhead = 0.0;

// Return prefix to print in front of each reported line
const char* Prefix() {
#ifdef NDEBUG
  return "";
#else
  return "DEBUG: ";
#endif
}

// TODO
// static internal::MallocCounter *benchmark_mc;

bool CpuScalingEnabled() {
  // On Linux, the CPUfreq subsystem exposes CPU information as files on the
  // local file system. If reading the exported files fails, then we may not be
  // running on Linux, so we silently ignore all the read errors.
  for (int cpu = 0, num_cpus = NumCPUs(); cpu < num_cpus; ++cpu) {
    std::stringstream ss;
    ss << "/sys/devices/system/cpu/cpu" << cpu << "/cpufreq/scaling_governor";
    std::string governor_file = ss.str();
    FILE* file = fopen(governor_file.c_str(), "r");
    if (!file) break;
    char buff[16];
    size_t bytes_read = fread(buff, 1, sizeof(buff), file);
    fclose(file);
    if (memprefix(buff, bytes_read, "performance") == NULL) return true;
  }
  return false;
}

// Given a collection of reports, computes their mean and stddev.
// REQUIRES: all runs in "reports" must be from the same benchmark.
void ComputeStats(const std::vector<BenchmarkReporter::Run>& reports,
                  BenchmarkReporter::Run* mean_data,
                  BenchmarkReporter::Run* stddev_data) {
  // Accumulators.
  Stat1_d real_accumulated_time_stat;
  Stat1_d cpu_accumulated_time_stat;
  Stat1_d items_per_second_stat;
  Stat1_d bytes_per_second_stat;
  Stat1_d iterations_stat;
  Stat1MinMax_d max_heapbytes_used_stat;

  // Populate the accumulators.
  for (std::vector<BenchmarkReporter::Run>::const_iterator it = reports.begin();
       it != reports.end(); ++it) {
    CHECK_EQ(reports[0].benchmark_name, it->benchmark_name);
    real_accumulated_time_stat +=
        Stat1_d(it->real_accumulated_time / it->iterations, it->iterations);
    cpu_accumulated_time_stat +=
        Stat1_d(it->cpu_accumulated_time / it->iterations, it->iterations);
    items_per_second_stat += Stat1_d(it->items_per_second, it->iterations);
    bytes_per_second_stat += Stat1_d(it->bytes_per_second, it->iterations);
    iterations_stat += Stat1_d(it->iterations, it->iterations);
    max_heapbytes_used_stat +=
        Stat1MinMax_d(it->max_heapbytes_used, it->iterations);
  }

  // Get the data from the accumulator to BenchmarkRunData's.  In the
  // computations below we must multiply by the number of iterations since
  // PrintRunData will divide by it.
  mean_data->benchmark_name = reports[0].benchmark_name + "_mean";
  mean_data->iterations = iterations_stat.Mean();
  mean_data->real_accumulated_time = real_accumulated_time_stat.Mean() *
                                     mean_data->iterations;
  mean_data->cpu_accumulated_time = cpu_accumulated_time_stat.Mean() *
                                    mean_data->iterations;
  mean_data->bytes_per_second = bytes_per_second_stat.Mean();
  mean_data->items_per_second = items_per_second_stat.Mean();
  mean_data->max_heapbytes_used = max_heapbytes_used_stat.Max();

  // Only add label to mean/stddev if it is same for all runs
  mean_data->report_label = reports[0].report_label;
  for (size_t i = 1; i < reports.size(); i++) {
    if (reports[i].report_label != reports[0].report_label) {
      mean_data->report_label = "";
      break;
    }
  }

  stddev_data->benchmark_name = reports[0].benchmark_name + "_stddev";
  stddev_data->report_label = mean_data->report_label;
  stddev_data->iterations = iterations_stat.StdDev();
  // The value of iterations_stat.StdDev() above may be 0 if all the repetitions
  // have the same number of iterations.  Blindly multiplying by 0 in the
  // computation of real/cpu_accumulated_time below would lead to 0/0 in
  // PrintRunData.  So we skip the multiplication in this case and PrintRunData
  // skips the division.
  if (stddev_data->iterations == 0) {
    stddev_data->real_accumulated_time = real_accumulated_time_stat.StdDev();
    stddev_data->cpu_accumulated_time = cpu_accumulated_time_stat.StdDev();
  } else {
    stddev_data->real_accumulated_time = real_accumulated_time_stat.StdDev() *
                                         stddev_data->iterations;
    stddev_data->cpu_accumulated_time = cpu_accumulated_time_stat.StdDev() *
                                        stddev_data->iterations;
  }
  stddev_data->bytes_per_second = bytes_per_second_stat.StdDev();
  stddev_data->items_per_second = items_per_second_stat.StdDev();
  stddev_data->max_heapbytes_used = max_heapbytes_used_stat.StdDev();
}
}  // namespace

namespace internal {

// Class for managing registered benchmarks.  Note that each registered
// benchmark identifies a family of related benchmarks to run.
class BenchmarkFamilies {
 public:
  static BenchmarkFamilies* GetInstance();

  // Registers a benchmark family and returns the index assigned to it.
  size_t AddBenchmark(Benchmark* family);

  // Unregisters a family at the given index.
  void RemoveBenchmark(size_t index);

  // Extract the list of benchmark instances that match the specified
  // regular expression.
  void FindBenchmarks(const std::string& re,
                      std::vector<Benchmark::Instance>* benchmarks);
 private:
  BenchmarkFamilies();
  ~BenchmarkFamilies();

  std::vector<Benchmark*> families_;
  std::mutex mutex_;
};

BenchmarkFamilies* BenchmarkFamilies::GetInstance() {
  static BenchmarkFamilies instance;
  return &instance;
}

BenchmarkFamilies::BenchmarkFamilies() { }

BenchmarkFamilies::~BenchmarkFamilies() {
  for (internal::Benchmark* family : families_) {
    delete family;
  }
}

size_t BenchmarkFamilies::AddBenchmark(Benchmark* family) {
  std::lock_guard<std::mutex> l(mutex_);
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
  std::lock_guard<std::mutex> l(mutex_);
  families_[index] = NULL;
  // Don't shrink families_ here, we might be called by the destructor of
  // BenchmarkFamilies which iterates over the vector.
}

void BenchmarkFamilies::FindBenchmarks(
    const std::string& spec,
    std::vector<Benchmark::Instance>* benchmarks) {
  // Make regular expression out of command-line flag
  Regex re;
  std::string re_error;
  if (!re.Init(spec, &re_error)) {
    std::cerr << "Could not compile benchmark re: " << re_error << std::endl;
    return;
  }

  std::lock_guard<std::mutex> l(mutex_);
  for (internal::Benchmark* family : families_) {
    if (family == nullptr) continue;  // Family was deleted

    // Match against filter.
    if (!re.Match(family->name_)) {
      VLOG(1) << "Skipping " << family->name_ << "\n";
      continue;
    }

    std::vector<Benchmark::Instance> instances;
    if (family->rangeX_.empty() && family->rangeY_.empty()) {
      instances = family->CreateBenchmarkInstances(
        Benchmark::kNoRangeIndex, Benchmark::kNoRangeIndex);
      std::copy(instances.begin(), instances.end(),
                std::back_inserter(*benchmarks));
    } else if (family->rangeY_.empty()) {
      for (size_t x = 0; x < family->rangeX_.size(); ++x) {
        instances = family->CreateBenchmarkInstances(
          x, Benchmark::kNoRangeIndex);
        std::copy(instances.begin(), instances.end(),
                  std::back_inserter(*benchmarks));
      }
    } else {
      for (size_t x = 0; x < family->rangeX_.size(); ++x) {
        for (size_t y = 0; y < family->rangeY_.size(); ++y) {
          instances = family->CreateBenchmarkInstances(x, y);
          std::copy(instances.begin(), instances.end(),
                    std::back_inserter(*benchmarks));
        }
      }
    }
  }
}

std::string ConsoleReporter::PrintMemoryUsage(double bytes) const {
  if (!get_memory_usage || bytes < 0.0) return "";

  std::stringstream ss;
  ss << " " << HumanReadableNumber(bytes) << "B peak-mem";
  return ss.str();
}

bool ConsoleReporter::ReportContext(const BenchmarkReporter::Context& context)
    const {
  name_field_width_ = context.name_field_width;

  std::cout << "Benchmarking on " << context.num_cpus << " X "
            << context.mhz_per_cpu << " MHz CPU"
            << ((context.num_cpus > 1) ? "s" : "") << "\n";

  int remainder_ms;
  std::cout << walltime::Print(walltime::Now(), "%Y/%m/%d-%H:%M:%S",
                               true,  // use local timezone
                               &remainder_ms) << "\n";

  // Show details of CPU model, caches, TLBs etc.
  //  if (!context.cpu_info.empty())
  //    std::cout << "CPU: " << context.cpu_info.c_str();

  if (context.cpu_scaling_enabled) {
    std::cerr << "CPU scaling is enabled: Benchmark timings may be noisy.\n";
  }

  int output_width = fprintf(stdout, "%s%-*s %10s %10s %10s\n",
                             Prefix(), int(name_field_width_), "Benchmark",
                             "Time(ns)", "CPU(ns)", "Iterations");
  std::cout << std::string(output_width - 1, '-').c_str() << "\n";

  return true;
}

void ConsoleReporter::ReportRuns(
    const std::vector<BenchmarkReporter::Run>& reports) const {
  for (std::vector<BenchmarkReporter::Run>::const_iterator it = reports.begin();
       it != reports.end(); ++it) {
    CHECK_EQ(reports[0].benchmark_name, it->benchmark_name);
    PrintRunData(*it);
  }

  // We don't report aggregated data if there was a single run.
  if (reports.size() < 2) return;

  BenchmarkReporter::Run mean_data;
  BenchmarkReporter::Run stddev_data;
  ComputeStats(reports, &mean_data, &stddev_data);

  PrintRunData(mean_data);
  PrintRunData(stddev_data);
}

void ConsoleReporter::PrintRunData(const BenchmarkReporter::Run& result) const {
  // Format bytes per second
  std::string rate;
  if (result.bytes_per_second > 0) {
    std::stringstream ss;
    ss << " " << HumanReadableNumber(result.bytes_per_second) << "B/s";
    rate = ss.str();
  }

  // Format items per second
  std::string items;
  if (result.items_per_second > 0) {
    std::stringstream ss;
    ss << " " << HumanReadableNumber(result.items_per_second) << " items/s";
    items = ss.str();
  }

  ColorPrintf(COLOR_DEFAULT, "%s", Prefix());
  ColorPrintf(COLOR_GREEN, "%-*s ",
              name_field_width_, result.benchmark_name.c_str());
  if (result.iterations == 0) {
    ColorPrintf(COLOR_YELLOW, "%10.0f %10.0f ",
                result.real_accumulated_time * 1e9,
                result.cpu_accumulated_time * 1e9);
  } else {
    ColorPrintf(COLOR_YELLOW, "%10.0f %10.0f ",
                (result.real_accumulated_time * 1e9) /
                    (static_cast<double>(result.iterations)),
                (result.cpu_accumulated_time * 1e9) /
                    (static_cast<double>(result.iterations)));
  }
  ColorPrintf(COLOR_CYAN, "%10lld", result.iterations);
  ColorPrintf(COLOR_DEFAULT, "%*s %*s %s %s\n",
              13, rate.c_str(),
              18, items.c_str(),
              result.report_label.c_str(),
              PrintMemoryUsage(result.max_heapbytes_used).c_str());
}

/* TODO(dominic)
void MemoryUsage() {
  // if (benchmark_mc) {
  //  benchmark_mc->Reset();
  //} else {
  get_memory_usage = true;
  //}
}
*/

void PrintUsageAndExit() {
  fprintf(stdout,
          "benchmark [--benchmark_filter=<regex>]\n"
          "          [--benchmark_iterations=<iterations>]\n"
          "          [--benchmark_min_time=<min_time>]\n"
          //"          [--benchmark_memory_usage]\n"
          "          [--benchmark_repetitions=<num_repetitions>]\n"
          "          [--color_print={true|false}]\n"
          "          [--v=<verbosity>]\n");
  exit(0);
}

void ParseCommandLineFlags(int* argc, const char** argv) {
  for (int i = 1; i < *argc; ++i) {
    if (ParseStringFlag(argv[i], "benchmark_filter", &FLAGS_benchmark_filter) ||
        ParseInt32Flag(argv[i], "benchmark_iterations",
                       &FLAGS_benchmark_iterations) ||
        ParseDoubleFlag(argv[i], "benchmark_min_time",
                        &FLAGS_benchmark_min_time) ||
        // TODO(dominic)
        //        ParseBoolFlag(argv[i], "gbenchmark_memory_usage",
        //                      &FLAGS_gbenchmark_memory_usage) ||
        ParseInt32Flag(argv[i], "benchmark_repetitions",
                       &FLAGS_benchmark_repetitions) ||
        ParseBoolFlag(argv[i], "color_print", &FLAGS_color_print) ||
        ParseInt32Flag(argv[i], "v", &FLAGS_v)) {
      for (int j = i; j != *argc; ++j) argv[j] = argv[j + 1];

      --(*argc);
      --i;
    } else if (IsFlag(argv[i], "help"))
      PrintUsageAndExit();
  }
}

}  // end namespace internal

// A clock that provides a fast mechanism to check if we're nearly done.
class State::FastClock {
 public:
  enum Type {
    REAL_TIME,
    CPU_TIME
  };
  explicit FastClock(Type type)
      : type_(type),
        approx_time_(NowMicros()),
        bg_done_(false),
        bg_(BGThreadWrapper, this) { }

  ~FastClock() {
    {
      std::unique_lock<std::mutex> l(bg_mutex_);
      bg_done_ = true;
      bg_cond_.notify_one();
    }
    bg_.join();
  }

  // Returns true if the current time is guaranteed to be past "when_micros".
  // This method is very fast.
  inline bool HasReached(int64_t when_micros) {
    return std::atomic_load(&approx_time_) >= when_micros;
  }

  // Returns the current time in microseconds past the epoch.
  int64_t NowMicros() const {
    double t = 0;
    switch (type_) {
      case REAL_TIME:
        t = walltime::Now();
        break;
      case CPU_TIME:
        t = MyCPUUsage() + ChildrenCPUUsage();
        break;
    }
    return static_cast<int64_t>(t * kNumMicrosPerSecond);
  }

  // Reinitialize if necessary (since clock type may be change once benchmark
  // function starts running - see UseRealTime).
  void InitType(Type type) {
    type_ = type;
    std::lock_guard<std::mutex> l(bg_mutex_);
    std::atomic_store(&approx_time_, NowMicros());
  }

 private:
  Type type_;
  std::atomic<int64_t> approx_time_;  // Last time measurement taken by bg_
  bool bg_done_;  // This is used to signal background thread to exit
  std::mutex bg_mutex_;
  std::condition_variable bg_cond_;
  std::thread bg_;  // Background thread that updates last_time_ once every ms

  static void* BGThreadWrapper(void* that) {
    ((FastClock*)that)->BGThread();
    return NULL;
  }

  void BGThread() {
    std::unique_lock<std::mutex> l(bg_mutex_);
    while (!bg_done_)
    {
      // Set timeout to 1 ms.
      bg_cond_.wait_for(l, std::chrono::milliseconds(1));
      std::atomic_store(&approx_time_, NowMicros());
    }
  }

  BENCHMARK_DISALLOW_COPY_AND_ASSIGN(FastClock);
};

struct State::ThreadStats {
  int64_t bytes_processed;
  int64_t items_processed;

  ThreadStats() { Reset(); }

  void Reset() {
    bytes_processed = 0;
    items_processed = 0;
  }

  void Add(const ThreadStats& other) {
    bytes_processed += other.bytes_processed;
    items_processed += other.items_processed;
  }
};

namespace internal {

// Information kept per benchmark we may want to run
struct Benchmark::Instance {
  Instance()
      : bm(nullptr),
        threads(1),
        rangeXset(false),
        rangeX(kNoRange),
        rangeYset(false),
        rangeY(kNoRange) {}

  std::string name;
  Benchmark* bm;
  int threads;  // Number of concurrent threads to use

  bool rangeXset;
  int rangeX;
  bool rangeYset;
  int rangeY;

  bool multithreaded() const { return !bm->thread_counts_.empty(); }
};

}  // end namespace internal

struct State::SharedState {
  const internal::Benchmark::Instance* instance;
  std::mutex mu;
  std::condition_variable cond;
  int starting;  // Number of threads that have entered STARTING state
  int stopping;  // Number of threads that have entered STOPPING state
  int exited;    // Number of threads that have complete exited
  int threads;   // Number of total threads that are running concurrently
  ThreadStats stats;
  std::vector<BenchmarkReporter::Run> runs;  // accumulated runs
  std::string label;

  explicit SharedState(const internal::Benchmark::Instance* b)
      : instance(b),
        starting(0),
        stopping(0),
        exited(0),
        threads(b == nullptr ? 1 : b->threads) { }

  BENCHMARK_DISALLOW_COPY_AND_ASSIGN(SharedState);
};

namespace internal {

Benchmark::Benchmark(const char* name, BenchmarkFunction f)
    : name_(name), function_(f) {
  registration_index_ = BenchmarkFamilies::GetInstance()->AddBenchmark(this);
}

Benchmark::~Benchmark() {
  BenchmarkFamilies::GetInstance()->RemoveBenchmark(registration_index_);
}

Benchmark* Benchmark::Arg(int x) {
  std::lock_guard<std::mutex> l(mutex_);
  rangeX_.push_back(x);
  return this;
}

Benchmark* Benchmark::Range(int start, int limit) {
  std::vector<int> arglist;
  AddRange(&arglist, start, limit, kRangeMultiplier);

  std::lock_guard<std::mutex> l(mutex_);
  for (size_t i = 0; i < arglist.size(); ++i) rangeX_.push_back(arglist[i]);
  return this;
}

Benchmark* Benchmark::DenseRange(int start, int limit) {
  CHECK_GE(start, 0);
  CHECK_LE(start, limit);
  std::lock_guard<std::mutex> l(mutex_);
  for (int arg = start; arg <= limit; ++arg) rangeX_.push_back(arg);
  return this;
}

Benchmark* Benchmark::ArgPair(int x, int y) {
  std::lock_guard<std::mutex> l(mutex_);
  rangeX_.push_back(x);
  rangeY_.push_back(y);
  return this;
}

Benchmark* Benchmark::RangePair(int lo1, int hi1, int lo2, int hi2) {
  std::vector<int> arglist1, arglist2;
  AddRange(&arglist1, lo1, hi1, kRangeMultiplier);
  AddRange(&arglist2, lo2, hi2, kRangeMultiplier);

  std::lock_guard<std::mutex> l(mutex_);
  rangeX_.resize(arglist1.size());
  std::copy(arglist1.begin(), arglist1.end(), rangeX_.begin());
  rangeY_.resize(arglist2.size());
  std::copy(arglist2.begin(), arglist2.end(), rangeY_.begin());
  return this;
}

Benchmark* Benchmark::Apply(void (*custom_arguments)(Benchmark* benchmark)) {
  custom_arguments(this);
  return this;
}

Benchmark* Benchmark::Threads(int t) {
  CHECK_GT(t, 0);
  std::lock_guard<std::mutex> l(mutex_);
  thread_counts_.push_back(t);
  return this;
}

Benchmark* Benchmark::ThreadRange(int min_threads, int max_threads) {
  CHECK_GT(min_threads, 0);
  CHECK_GE(max_threads, min_threads);

  std::lock_guard<std::mutex> l(mutex_);
  AddRange(&thread_counts_, min_threads, max_threads, 2);
  return this;
}

Benchmark* Benchmark::ThreadPerCpu() {
  std::lock_guard<std::mutex> l(mutex_);
  thread_counts_.push_back(NumCPUs());
  return this;
}

void Benchmark::AddRange(std::vector<int>* dst, int lo, int hi, int mult) {
  CHECK_GE(lo, 0);
  CHECK_GE(hi, lo);

  // Add "lo"
  dst->push_back(lo);

  // Now space out the benchmarks in multiples of "mult"
  for (int32_t i = 1; i < std::numeric_limits<int32_t>::max() / mult;
       i *= mult) {
    if (i >= hi) break;
    if (i > lo) dst->push_back(i);
  }
  // Add "hi" (if different from "lo")
  if (hi != lo) dst->push_back(hi);
}

std::vector<Benchmark::Instance> Benchmark::CreateBenchmarkInstances(
    size_t rangeXindex, size_t rangeYindex) {
  // Special list of thread counts to use when none are specified
  std::vector<int> one_thread;
  one_thread.push_back(1);

  std::vector<Benchmark::Instance> instances;

  const bool is_multithreaded = (!thread_counts_.empty());
  const std::vector<int>& thread_counts =
      (is_multithreaded ? thread_counts_ : one_thread);
  for (int num_threads : thread_counts) {
    Instance instance;
    instance.name = name_;
    instance.bm = this;
    instance.threads = num_threads;

    if (rangeXindex != kNoRangeIndex) {
      instance.rangeX = rangeX_[rangeXindex];
      instance.rangeXset = true;
      AppendHumanReadable(instance.rangeX, &instance.name);
    }
    if (rangeYindex != kNoRangeIndex) {
      instance.rangeY = rangeY_[rangeYindex];
      instance.rangeYset = true;
      AppendHumanReadable(instance.rangeY, &instance.name);
    }

    // Add the number of threads used to the name
    if (is_multithreaded) {
      std::stringstream ss;
      ss << "/threads:" << instance.threads;
      instance.name += ss.str();
    }

    instances.push_back(instance);
  }

  return instances;
}

void Benchmark::MeasureOverhead() {
  State::FastClock clock(State::FastClock::CPU_TIME);
  State::SharedState state(nullptr);
  State runner(&clock, &state, 0);
  while (runner.KeepRunning()) {
  }
  overhead = state.runs[0].real_accumulated_time /
             static_cast<double>(state.runs[0].iterations);
  VLOG(1) << "Per-iteration overhead for doing nothing: " << overhead << "\n";
}

void Benchmark::RunInstance(const Instance& b, const BenchmarkReporter* br) {
  use_real_time = false;
  running_benchmark = true;
  // get_memory_usage = FLAGS_gbenchmark_memory_usage;
  State::FastClock clock(State::FastClock::CPU_TIME);

  // Initialize the test runners.
  State::SharedState state(&b);
  {
    std::vector<std::unique_ptr<State>> runners;
    for (int i = 0; i < b.threads; ++i)
      runners.push_back(std::unique_ptr<State>(new State(&clock, &state, i)));

    // Run them all.
    for (int i = 0; i < b.threads; ++i) {
      if (b.multithreaded())
        runners[i]->RunAsThread();
      else
        runners[i]->Run();
    }
    if (b.multithreaded()) {
      for (int i = 0; i < b.threads; ++i) runners[i]->Wait();
    }
  }
  /*
    double mem_usage = 0;
    if (get_memory_usage) {
      // Measure memory usage
      Notification mem_done;
      BenchmarkRun mem_run;
      BenchmarkRun::SharedState mem_shared(&b, 1);
      mem_run.Init(&clock, &mem_shared, 0);
      {
        testing::MallocCounter mc(testing::MallocCounter::THIS_THREAD_ONLY);
        benchmark_mc = &mc;
        mem_run.Run(&mem_done);
        mem_done.WaitForNotification();
        benchmark_mc = NULL;
        mem_usage = mc.PeakHeapGrowth();
      }
    }
  */
  running_benchmark = false;

  for (BenchmarkReporter::Run& report : state.runs) {
    double seconds = (use_real_time ? report.real_accumulated_time
                                    : report.cpu_accumulated_time);
    report.benchmark_name = b.name;
    report.report_label = state.label;
    report.bytes_per_second = state.stats.bytes_processed / seconds;
    report.items_per_second = state.stats.items_processed / seconds;
    report.max_heapbytes_used = MeasurePeakHeapMemory(b);
  }

  br->ReportRuns(state.runs);
}

// Run the specified benchmark, measure its peak memory usage, and
// return the peak memory usage.
double Benchmark::MeasurePeakHeapMemory(const Instance&) {
  if (!get_memory_usage) return 0.0;
  double bytes = 0.0;
  /*  TODO(dominich)
   // Should we do multi-threaded runs?
   const int num_threads = 1;
   const int num_iters = 1;
   {
 //    internal::MallocCounter mc(internal::MallocCounter::THIS_THREAD_ONLY);
     running_benchmark = true;
     timer_manager = new TimerManager(1, NULL);
 //    benchmark_mc = &mc;
     timer_manager->StartTimer();

     b.Run(num_iters);

     running_benchmark = false;
     delete timer_manager;
     timer_manager = NULL;
 //    benchmark_mc = NULL;
 //    bytes = mc.PeakHeapGrowth();
   }
   */
  return bytes;
}

}  // end namespace internal

State::State(FastClock* clock, SharedState* s, int t)
    : thread_index(t),
      state_(STATE_INITIAL),
      clock_(clock),
      shared_(s),
      iterations_(0),
      start_cpu_(0.0),
      start_time_(0.0),
      stop_time_micros_(0.0),
      start_pause_cpu_(0.0),
      pause_cpu_time_(0.0),
      start_pause_real_(0.0),
      pause_real_time_(0.0),
      total_iterations_(0),
      interval_micros_(static_cast<int64_t>(kNumMicrosPerSecond *
                                            FLAGS_benchmark_min_time /
                                            FLAGS_benchmark_repetitions)),
      is_continuation_(false),
      stats_(new ThreadStats()) {
  CHECK(clock != nullptr);
  CHECK(s != nullptr);
}

bool State::KeepRunning() {
  // Fast path
  if ((FLAGS_benchmark_iterations == 0 &&
       !clock_->HasReached(stop_time_micros_ +
                           kNumMicrosPerSecond * pause_real_time_)) ||
      iterations_ < FLAGS_benchmark_iterations) {
    ++iterations_;
    return true;
  }

  // To block thread 0 until all other threads exit, we have a signal exit
  // point for KeepRunning() to return false.  The fast path above always
  // returns true.
  bool ret = false;
  switch (state_) {
    case STATE_INITIAL:
      ret = StartRunning();
      break;
    case STATE_STARTING:
      CHECK(false);
      ret = true;
      break;
    case STATE_RUNNING:
      ret = FinishInterval();
      break;
    case STATE_STOPPING:
      ret = MaybeStop();
      break;
    case STATE_STOPPED:
      CHECK(false);
      ret = true;
      break;
  }

  if (!ret && shared_->threads > 1 && thread_index == 0){
    std::unique_lock<std::mutex> l(shared_->mu);

    // Block until all other threads have exited.  We can then safely cleanup
    // without other threads continuing to access shared variables inside the
    // user-provided run function.
    while (shared_->exited < shared_->threads - 1) {
      shared_->cond.wait(l);
    }
  }

  if (ret) {
    ++iterations_;
  }
  return ret;
}

void State::PauseTiming() {
  start_pause_cpu_ = MyCPUUsage() + ChildrenCPUUsage();
  start_pause_real_ = walltime::Now();
}

void State::ResumeTiming() {
  pause_cpu_time_ += MyCPUUsage() + ChildrenCPUUsage() - start_pause_cpu_;
  pause_real_time_ += walltime::Now() - start_pause_real_;
}

void State::SetBytesProcessed(int64_t bytes) {
  CHECK_EQ(STATE_STOPPED, state_);
  std::lock_guard<std::mutex> l(shared_->mu);
  stats_->bytes_processed = bytes;
}

void State::SetItemsProcessed(int64_t items) {
  CHECK_EQ(STATE_STOPPED, state_);
  std::lock_guard<std::mutex> l(shared_->mu);
  stats_->items_processed = items;
}

void State::SetLabel(const std::string& label) {
  CHECK_EQ(STATE_STOPPED, state_);
  std::lock_guard<std::mutex> l(shared_->mu);
  shared_->label = label;
}

int State::range_x() const {
  CHECK(shared_->instance->rangeXset);
  /*
  <<
      "Failed to get range_x as it was not set. Did you register your "
      "benchmark with a range parameter?";
      */
  return shared_->instance->rangeX;
}

int State::range_y() const {
  CHECK(shared_->instance->rangeYset);
  /* <<
       "Failed to get range_y as it was not set. Did you register your "
       "benchmark with a range parameter?";
       */
  return shared_->instance->rangeY;
}

bool State::StartRunning() {
  bool last_thread = false;
  {
    std::lock_guard<std::mutex> l(shared_->mu);
    CHECK_EQ(state_, STATE_INITIAL);
    state_ = STATE_STARTING;
    is_continuation_ = false;
    CHECK_LT(shared_->starting, shared_->threads);
    ++shared_->starting;
    last_thread = shared_->starting == shared_->threads;
  }

  if (last_thread) {
    clock_->InitType(use_real_time ? FastClock::REAL_TIME
                                   : FastClock::CPU_TIME);
    {
      std::lock_guard<std::mutex> l(starting_mutex);
      starting_cv.notify_all();
    }
  } else {
    std::unique_lock<std::mutex> l(starting_mutex);
    starting_cv.wait(l);
  }
  CHECK_EQ(state_, STATE_STARTING);
  state_ = STATE_RUNNING;

  NewInterval();
  return true;
}

void State::NewInterval() {
  stop_time_micros_ = clock_->NowMicros() + interval_micros_;
  if (!is_continuation_) {
    VLOG(1) << "Starting new interval; stopping in " << interval_micros_
            << "\n";
    iterations_ = 0;
    pause_cpu_time_ = 0;
    pause_real_time_ = 0;
    start_cpu_ = MyCPUUsage() + ChildrenCPUUsage();
    start_time_ = walltime::Now();
  } else {
    VLOG(1) << "Continuing interval; stopping in " << interval_micros_
            << "\n";
  }
}

bool State::FinishInterval() {
  if ((FLAGS_benchmark_iterations != 0 &&
       iterations_ <
           FLAGS_benchmark_iterations / FLAGS_benchmark_repetitions) ||
      iterations_ < 1) {
    interval_micros_ *= 2;
    VLOG(1) << "Not enough iterations in interval; "
            << "Trying again for " << interval_micros_ << " useconds.\n";
    is_continuation_ = false;
    NewInterval();
    return true;
  }

  BenchmarkReporter::Run data;
  data.iterations = iterations_;
  data.thread_index = thread_index;

  const double accumulated_time = walltime::Now() - start_time_;
  const double total_overhead = overhead * iterations_;
  CHECK_LT(pause_real_time_, accumulated_time);
  CHECK_LT(pause_real_time_ + total_overhead, accumulated_time);
  data.real_accumulated_time =
      accumulated_time - (pause_real_time_ + total_overhead);
  data.cpu_accumulated_time = (MyCPUUsage() + ChildrenCPUUsage()) -
                              (pause_cpu_time_ + start_cpu_);
  total_iterations_ += iterations_;

  bool keep_going = false;
  {
    std::lock_guard<std::mutex> l(shared_->mu);

    // Either replace the last or add a new data point.
    if (is_continuation_)
      shared_->runs.back() = data;
    else
      shared_->runs.push_back(data);

    if (FLAGS_benchmark_iterations != 0) {
      // If we need more iterations, run another interval as a continuation.
      keep_going = total_iterations_ < FLAGS_benchmark_iterations;
      is_continuation_ = keep_going;
    } else {
      // If this is a repetition, run another interval as a new data point.
      keep_going = shared_->runs.size() <
                   static_cast<size_t>(FLAGS_benchmark_repetitions);
      is_continuation_ = !keep_going;
    }

    if (!keep_going) {
      ++shared_->stopping;
      if (shared_->stopping < shared_->threads) {
        // Other threads are still running, so continue running but without
        // timing to present an expected background load to the other threads.
        state_ = STATE_STOPPING;
        keep_going = true;
      } else {
        state_ = STATE_STOPPED;
      }
    }
  }

  if (state_ == STATE_RUNNING) NewInterval();
  return keep_going;
}

bool State::MaybeStop() {
  std::lock_guard<std::mutex> l(shared_->mu);
  if (shared_->stopping < shared_->threads) {
    CHECK_EQ(state_, STATE_STOPPING);
    return true;
  }
  state_ = STATE_STOPPED;
  return false;
}

void State::Run() {
  stats_->Reset();
  shared_->instance->bm->function_(*this);
  {
    std::lock_guard<std::mutex> l(shared_->mu);
    shared_->stats.Add(*stats_);
  }
}

void State::RunAsThread() {
  thread_ = std::thread(State::RunWrapper, this);
}

void State::Wait() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

// static
void* State::RunWrapper(void* arg) {
  State* that = (State*)arg;
  CHECK(that != nullptr);
  that->Run();

  std::lock_guard<std::mutex> l(that->shared_->mu);

  that->shared_->exited++;
  if (that->thread_index > 0 &&
      that->shared_->exited == that->shared_->threads - 1) {
    // All threads but thread 0 have exited the user-provided run function.
    // Thread 0 can now wake up and exit.
    that->shared_->cond.notify_one();
  }

  return nullptr;
}

namespace internal {

void RunMatchingBenchmarks(const std::string& spec,
                           const BenchmarkReporter* reporter) {
  if (spec.empty()) return;

  std::vector<internal::Benchmark::Instance> benchmarks;
  BenchmarkFamilies::GetInstance()->FindBenchmarks(spec, &benchmarks);

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
  //  context.cpu_info = base::CompactCPUIDInfoString();
  context.cpu_scaling_enabled = CpuScalingEnabled();
  context.name_field_width = name_field_width;

  if (reporter->ReportContext(context))
    for (internal::Benchmark::Instance& benchmark : benchmarks)
      Benchmark::RunInstance(benchmark, reporter);
}

void FindMatchingBenchmarkNames(const std::string& spec,
                                std::vector<std::string>* benchmark_names) {
  if (spec.empty()) return;

  std::vector<internal::Benchmark::Instance> benchmarks;
  BenchmarkFamilies::GetInstance()->FindBenchmarks(spec, &benchmarks);
  std::transform(benchmarks.begin(), benchmarks.end(), benchmark_names->begin(),
                 [](const internal::Benchmark::Instance& b) { return b.name; });
}

}  // end namespace internal

void RunSpecifiedBenchmarks(const BenchmarkReporter* reporter /*= nullptr*/) {
  std::string spec = FLAGS_benchmark_filter;
  if (spec.empty() || spec == "all")
    spec = ".";  // Regexp that matches all benchmarks
  internal::ConsoleReporter default_reporter;
  internal::RunMatchingBenchmarks(
      spec, reporter == nullptr ? &default_reporter : reporter);
}

void UseRealTime() { use_real_time = true; }

void Initialize(int* argc, const char** argv) {
  internal::ParseCommandLineFlags(argc, argv);
  internal::SetLogLevel(FLAGS_v);
  // Ensure walltime is initialized by a single thread by forcing the
  // initialization.
  walltime::Now();
  internal::Benchmark::MeasureOverhead();
}

}  // end namespace benchmark
