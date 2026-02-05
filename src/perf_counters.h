// Copyright 2021 Google Inc. All rights reserved.
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

#ifndef BENCHMARK_PERF_COUNTERS_H
#define BENCHMARK_PERF_COUNTERS_H

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "benchmark/benchmark.h"
#include "check.h"
#include "log.h"
#include "mutex.h"

#ifndef BENCHMARK_OS_WINDOWS
#include <unistd.h>
#endif

#if defined(_MSC_VER)
#pragma warning(push)
// C4251: <symbol> needs to have dll-interface to be used by clients of class
#pragma warning(disable : 4251)
#endif

namespace benchmark {
namespace internal {

// Typically, we only read a small number of counters. There is also a
// specific format when reading multiple counters with one syscall
// (which is desirable). PerfCounterValues abstracts these details.
// The implementation ensures the storage is inlined, and allows 0-based
// indexing into the counter values.
// The object is used in conjunction with a PerfCounters object, by passing it
// to Snapshot(). The Read() method reads in the appropriate fields into
// the values buffer such that all user accesses through the [] operator are
// correct.
// The [] operator estimates the true value of the counter using the
// time_enabled and time_running values (which will be equal if there is no
// multiplexing, i.e only a single group).
class BENCHMARK_EXPORT PerfCounterValues {
 public:
  explicit PerfCounterValues(size_t nr_counters) : nr_counters_(nr_counters) {
    BM_CHECK_LE(nr_counters_, kMaxCounters);
  }

  double operator[](size_t pos) const { return values_[pos].GetEstimate(); }

  PerfCounterValues operator-=(const PerfCounterValues& counter_values) {
    for (size_t i = 0; i < nr_counters_; i++) {
      values_[i] -= counter_values.values_[i];
    }

    return *this;
  }

  // Increased the maximum to 32 only since the buffer
  // is std::array<> backed
  static constexpr size_t kMaxCounters = 32;

 private:
  // Represents the value of a counter.
  // time_enabled_ = time_running_ if there is no multiplexing,
  // i.e only a single group.
  class Value {
   public:
    void Set(uint64_t time_enabled, uint64_t time_running, uint64_t value) {
      time_enabled_ = time_enabled;
      time_running_ = time_running;
      value_ = value;
    }

    double GetEstimate() const {
      return static_cast<double>(value_ * time_enabled_) /
             static_cast<double>(time_running_);
    }

    Value operator-=(const Value& value) {
      time_enabled_ -= value.time_enabled_;
      time_running_ -= value.time_running_;
      value_ -= value.value_;

      return *this;
    }

   private:
    uint64_t time_enabled_;
    uint64_t time_running_;
    uint64_t value_;
  };

  friend class PerfCounters;

  // This reading is complex and as the goal of this class is to
  // abstract away the intrincacies of the reading process, this is
  // a better place for it
  size_t Read(const std::vector<int>& leaders);

  std::array<Value, kMaxCounters> values_;
  const size_t nr_counters_;
};

// Collect PMU counters. The object, once constructed, is ready to be used by
// calling read(). PMU counter collection is enabled from the time create() is
// called, to obtain the object, until the object's destructor is called.
class BENCHMARK_EXPORT PerfCounters final {
 public:
  // True iff this platform supports performance counters.
  static const bool kSupported;

  // Returns an empty object
  static PerfCounters NoCounters() { return PerfCounters(); }

  ~PerfCounters() { CloseCounters(); }
  PerfCounters() = default;
  PerfCounters(PerfCounters&&) = default;
  PerfCounters(const PerfCounters&) = delete;
  PerfCounters& operator=(PerfCounters&&) noexcept;
  PerfCounters& operator=(const PerfCounters&) = delete;

  // Platform-specific implementations may choose to do some library
  // initialization here.
  static bool Initialize();

  // Check if the given counter is supported, if the app wants to
  // check before passing
  static bool IsCounterSupported(const std::string& name);

  // Return a PerfCounters object ready to read the counters with the names
  // specified. The values are user-mode only. The counter name format is
  // implementation and OS specific.
  // In case of failure, this method will in the worst case return an
  // empty object whose state will still be valid.
  static PerfCounters Create(const std::vector<std::string>& counter_names);

  // Take a snapshot of the current value of the counters into the provided
  // valid PerfCounterValues storage. The values are populated such that:
  // names()[i]'s value is (*values)[i]
  BENCHMARK_ALWAYS_INLINE bool Snapshot(PerfCounterValues* values) const {
#ifndef BENCHMARK_OS_WINDOWS
    assert(values != nullptr);
    return values->Read(leader_ids_) == counter_ids_.size();
#else
    (void)values;
    return false;
#endif
  }

  const std::vector<std::string>& names() const { return counter_names_; }
  size_t num_counters() const { return counter_names_.size(); }

 private:
  PerfCounters(const std::vector<std::string>& counter_names,
               std::vector<int>&& counter_ids, std::vector<int>&& leader_ids)
      : counter_ids_(std::move(counter_ids)),
        leader_ids_(std::move(leader_ids)),
        counter_names_(counter_names) {}

  void CloseCounters() const;

  std::vector<int> counter_ids_;
  std::vector<int> leader_ids_;
  std::vector<std::string> counter_names_;
};

// Typical usage of the above primitives.
class BENCHMARK_EXPORT PerfCountersMeasurement final {
 public:
  PerfCountersMeasurement(const std::vector<std::string>& counter_names);

  size_t num_counters() const { return counters_.num_counters(); }

  std::vector<std::string> names() const { return counters_.names(); }

  BENCHMARK_ALWAYS_INLINE bool Start() {
    if (num_counters() == 0) return true;
    // Tell the compiler to not move instructions above/below where we take
    // the snapshot.
    ClobberMemory();
    valid_read_ &= counters_.Snapshot(&start_values_);
    ClobberMemory();

    return valid_read_;
  }

  BENCHMARK_ALWAYS_INLINE bool Stop(
      std::vector<std::pair<std::string, double>>& measurements) {
    if (num_counters() == 0) return true;
    // Tell the compiler to not move instructions above/below where we take
    // the snapshot.
    ClobberMemory();
    valid_read_ &= counters_.Snapshot(&end_values_);
    ClobberMemory();

    end_values_ -= start_values_;
    for (size_t i = 0; i < counters_.names().size(); ++i) {
      measurements.push_back({counters_.names()[i], end_values_[i]});
    }

    return valid_read_;
  }

 private:
  PerfCounters counters_;
  bool valid_read_ = true;
  PerfCounterValues start_values_;
  PerfCounterValues end_values_;
};

}  // namespace internal
}  // namespace benchmark

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif  // BENCHMARK_PERF_COUNTERS_H
