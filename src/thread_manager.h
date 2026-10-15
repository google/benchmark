#ifndef BENCHMARK_THREAD_MANAGER_H
#define BENCHMARK_THREAD_MANAGER_H

#include <atomic>
#include <vector>

#include "benchmark/counter.h"
#include "benchmark/statistics.h"
#include "benchmark/types.h"
#include "mutex.h"

namespace benchmark {
namespace internal {

class ThreadManager {
 public:
  explicit ThreadManager(int num_threads, bool collect_per_thread = false)
      : start_stop_barrier_(num_threads) {
    if (collect_per_thread && num_threads > 1) {
      per_thread_results.resize(static_cast<size_t>(num_threads));
    }
  }

  Mutex& GetBenchmarkMutex() const RETURN_CAPABILITY(benchmark_mutex_) {
    return benchmark_mutex_;
  }

  bool StartStopBarrier() { return start_stop_barrier_.wait(); }

  void NotifyThreadComplete() { start_stop_barrier_.removeThread(); }

  struct Result {
    IterationCount iterations = 0;
    double real_time_used = 0;
    double cpu_time_used = 0;
    double manual_time_used = 0;
    int64_t complexity_n = 0;
    std::string report_label_;
    std::string skip_message_;
    internal::Skipped skipped_ = internal::NotSkipped;
    UserCounters counters;
  };
  GUARDED_BY(GetBenchmarkMutex()) Result results;
  // Populated only when collect_per_thread is true and num_threads > 1.
  GUARDED_BY(GetBenchmarkMutex()) std::vector<Result> per_thread_results;

 private:
  mutable Mutex benchmark_mutex_;
  Barrier start_stop_barrier_;
};

}  // namespace internal
}  // namespace benchmark

#endif  // BENCHMARK_THREAD_MANAGER_H
