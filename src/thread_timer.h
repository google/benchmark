#ifndef BENCHMARK_THREAD_TIMER_H
#define BENCHMARK_THREAD_TIMER_H

#include "check.h"
#include "timers.h"

namespace benchmark {
namespace internal {

class ThreadTimer {
 public:
  ThreadTimer() = default;

  // Called by each thread
  void StartTimer() {
    running_ = true;
    start_real_time_ = ChronoClockNow();
    start_cpu_time_ = ThreadCPUUsage();
  }

  // Called by each thread
  void WriteTimer() {
     CHECK(running_);
     double real_time = ChronoClockNow() - start_real_time_;
     double cpu_time = std::max<double>(ThreadCPUUsage() - start_cpu_time_, 0);

     real_time_used_ += real_time;
     // Floating point error can result in the subtraction producing a negative
     // time. Guard against that.
     cpu_time_used_ += cpu_time;

     // Squared times for standard deviation calculation
     real_squared_time_used_ += real_time * real_time;
     cpu_squared_time_used_ += cpu_time * cpu_time;

     //min and max calculation
     min_real_time_used_ = std::min(real_time, min_real_time_used_);
     max_real_time_used_ = std::max(real_time, max_real_time_used_);
     min_cpu_time_used_ = std::min(cpu_time, min_cpu_time_used_);
     max_cpu_time_used_ = std::max(cpu_time, max_cpu_time_used_);

     start_real_time_ = ChronoClockNow();
     start_cpu_time_ = ThreadCPUUsage();
  }

  // Called by each thread
  void StopTimer() {
    CHECK(running_);
    running_ = false;
    real_time_used_ += ChronoClockNow() - start_real_time_;
    // Floating point error can result in the subtraction producing a negative
    // time. Guard against that.
    cpu_time_used_ += std::max<double>(ThreadCPUUsage() - start_cpu_time_, 0);
  }

  // Called by each thread
  void SetIterationTime(double seconds) {
     manual_time_used_ += seconds;
     manual_squared_time_used_ += seconds * seconds;
     min_manual_time_used_ = std::min(min_manual_time_used_, seconds);
     max_manual_time_used_ = std::max(max_manual_time_used_, seconds);
  }

  bool running() const { return running_; }

  // REQUIRES: timer is not running
  double real_time_used() {
    CHECK(!running_);
    return real_time_used_;
  }

  // REQUIRES: timer is not running
  double cpu_time_used() {
    CHECK(!running_);
    return cpu_time_used_;
  }

  // REQUIRES: timer is not running
  double manual_time_used() {
    CHECK(!running_);
    return manual_time_used_;
  }

  // REQUIRES: timer is not running
  double real_squared_time_used() {
     CHECK(!running_);
     return real_squared_time_used_;
  }

  // REQUIRES: timer is not running
  double cpu_squared_time_used() {
     CHECK(!running_);
     return cpu_squared_time_used_;
  }

  // REQUIRES: timer is not running
  double manual_squared_time_used() {
     CHECK(!running_);
     return manual_squared_time_used_;
  }

  // REQUIRES: timer is not running
  double min_real_time_used() {
     CHECK(!running_);
     return min_real_time_used_;
  }

  // REQUIRES: timer is not running
  double max_real_time_used() {
     CHECK(!running_);
     return max_real_time_used_;
  }

  // REQUIRES: timer is not running
  double min_cpu_time_used() {
     CHECK(!running_);
     return min_cpu_time_used_;
  }

  // REQUIRES: timer is not running
  double max_cpu_time_used() {
     CHECK(!running_);
     return max_cpu_time_used_;
  }

  // REQUIRES: timer is not running
  double min_manual_time_used() {
     CHECK(!running_);
     return min_manual_time_used_;
  }

  // REQUIRES: timer is not running
  double max_manual_time_used() {
     CHECK(!running_);
     return max_manual_time_used_;
  }

 private:
  bool running_ = false;        // Is the timer running
  double start_real_time_ = 0;  // If running_
  double start_cpu_time_ = 0;   // If running_

  // Accumulated time so far (does not contain current slice if running_)
  double real_time_used_ = 0;
  double cpu_time_used_ = 0;

  // Manually set iteration time. User sets this with SetIterationTime(seconds).
  double manual_time_used_ = 0;

  // Accumulated squared time for standard deviation calculation
  double real_squared_time_used_ = 0;
  double cpu_squared_time_used_ = 0;
  double manual_squared_time_used_ = 0;

  // min / max
  double min_real_time_used_ = std::numeric_limits<double>::max();
  double max_real_time_used_ = 0.;
  double min_cpu_time_used_ = std::numeric_limits<double>::max();
  double max_cpu_time_used_ = 0.;
  double min_manual_time_used_ = std::numeric_limits<double>::max();
  double max_manual_time_used_ = 0.;
};

}  // namespace internal
}  // namespace benchmark

#endif  // BENCHMARK_THREAD_TIMER_H
