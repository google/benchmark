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

#include "perf_counters.h"

#include <cstring>
#include <memory>
#include <vector>

#if defined HAVE_LIBPFM
#include <unordered_map>

#include "perfmon/pfmlib.h"
#include "perfmon/pfmlib_perf_event.h"
#endif

namespace benchmark {
namespace internal {

#if defined HAVE_LIBPFM

class SinglePMURegistry {
 public:
  ~SinglePMURegistry() = default;
  SinglePMURegistry(SinglePMURegistry&&) = default;
  SinglePMURegistry(const SinglePMURegistry&) = delete;
  SinglePMURegistry& operator=(SinglePMURegistry&&) noexcept;
  SinglePMURegistry& operator=(const SinglePMURegistry&) = delete;

  SinglePMURegistry(pfm_pmu_t pmu_id)
      : pmu_id_(pmu_id), available_counters_(0), available_fixed_counters_(0) {
    pfm_pmu_info_t pmu_info{};
    const auto pfm_pmu = pfm_get_pmu_info(pmu_id, &pmu_info);

    if (pfm_pmu != PFM_SUCCESS) {
      GetErrorLogInstance() << "Unknown pmu: " << pmu_id << "\n";
      return;
    }

    name_ = pmu_info.name;
    desc_ = pmu_info.desc;
    available_counters_ = pmu_info.num_cntrs;
    available_fixed_counters_ = pmu_info.num_fixed_cntrs;

    BM_VLOG(1) << "PMU: " << pmu_id << " " << name_ << " " << desc_ << "\n";
    BM_VLOG(1) << "     counters: " << available_counters_
               << " fixed: " << available_fixed_counters_ << "\n";
  }

  const char* name() const { return name_; }

  bool AddCounter(int event_id) {
    pfm_event_info_t info{};
    const auto pfm_event_info =
        pfm_get_event_info(event_id, PFM_OS_PERF_EVENT, &info);

    if (pfm_event_info != PFM_SUCCESS) {
      GetErrorLogInstance() << "Unknown event id: " << event_id << "\n";
      return false;
    }

    assert(info.pmu == pmu_id_);

    if (counter_ids_.find(event_id) != counter_ids_.end()) return true;

    assert(std::numeric_limits<int>::max() > counter_ids_.size());
    if (static_cast<int>(counter_ids_.size()) >= available_counters_ - 1) {
      GetErrorLogInstance() << "Maximal number of counters for PMU " << name_
                            << " (" << available_counters_ << ") reached.\n";
      return false;
    }

    counter_ids_.emplace(event_id, info.code);

    BM_VLOG(2) << "Registered counter: " << event_id << " (" << info.name
               << " - " << info.desc << ") in pmu " << name_ << " ("
               << counter_ids_.size() << "/" << available_counters_ << "\n";

    return true;
  }

 private:
  pfm_pmu_t pmu_id_;
  const char* name_;
  const char* desc_;
  std::unordered_map<int, uint64_t> counter_ids_;
  std::unordered_map<int, uint64_t> fixed_counter_ids_;
  int available_counters_;
  int available_fixed_counters_;
};

class PMURegistry {
 public:
  ~PMURegistry() = default;
  PMURegistry(PMURegistry&&) = default;
  PMURegistry(const PMURegistry&) = delete;
  PMURegistry& operator=(PMURegistry&&) noexcept;
  PMURegistry& operator=(const PMURegistry&) = delete;
  PMURegistry() {}

  bool EnlistCounter(const std::string& name,
                     struct perf_event_attr& attr_base) {
    attr_base.size = sizeof(attr_base);
    pfm_perf_encode_arg_t encoding{};
    encoding.attr = &attr_base;

    const auto pfm_get = pfm_get_os_event_encoding(
        name.c_str(), PFM_PLM3, PFM_OS_PERF_EVENT, &encoding);
    if (pfm_get != PFM_SUCCESS) {
      GetErrorLogInstance() << "Unknown counter name: " << name << "\n";
      return false;
    }

    pfm_event_info_t info{};
    const auto pfm_info =
        pfm_get_event_info(encoding.idx, PFM_OS_PERF_EVENT, &info);
    if (pfm_info != PFM_SUCCESS) {
      GetErrorLogInstance()
          << "Unknown counter idx: " << encoding.idx << "(" << name << ")\n";
      return false;
    }

    // Spin-up a new per-PMU sub-registry if needed
    if (pmu_registry_.find(info.pmu) == pmu_registry_.end()) {
      pmu_registry_.emplace(info.pmu, SinglePMURegistry(info.pmu));
    }

    auto& single_pmu = pmu_registry_.find(info.pmu)->second;

    return single_pmu.AddCounter(info.idx);
  }

 private:
  std::unordered_map<pfm_pmu_t, SinglePMURegistry> pmu_registry_;
};

const bool PerfCounters::kSupported = true;

bool PerfCounters::Initialize() { return pfm_initialize() == PFM_SUCCESS; }

PerfCounters PerfCounters::Create(
    const std::vector<std::string>& counter_names) {
  if (counter_names.empty()) {
    return NoCounters();
  }

  std::vector<int> counter_ids(counter_names.size());
  PMURegistry registry{};

  for (size_t i = 0; i < counter_names.size(); ++i) {
    const auto& name = counter_names[i];
    if (name.empty()) {
      GetErrorLogInstance() << "A counter name was the empty string\n";
      return NoCounters();
    }

    struct perf_event_attr attr {};
    auto ok = registry.EnlistCounter(name, attr);

    if (!ok) {
      GetErrorLogInstance() << "Failed to register counter: " << name << "\n";
      return NoCounters();
    }

    const bool is_first = i == 0;
    const int group_id = !is_first ? counter_ids[0] : -1;

    attr.disabled = is_first;
    // Note: the man page for perf_event_create suggests inherit = true and
    // read_format = PERF_FORMAT_GROUP don't work together, but that's not the
    // case.
    attr.inherit = true;
    attr.pinned = is_first;
    attr.exclude_kernel = true;
    attr.exclude_user = false;
    attr.exclude_hv = true;
    // Read all counters in one read.
    attr.read_format = PERF_FORMAT_GROUP;

    int id = -1;
    static constexpr size_t kNrOfSyscallRetries = 5;
    // Retry syscall as it was interrupted often (b/64774091).
    for (size_t num_retries = 0; num_retries < kNrOfSyscallRetries;
         ++num_retries) {
      id = perf_event_open(&attr, 0, -1, group_id, 0);
      if (id >= 0 || errno != EINTR) {
        break;
      }
    }
    if (id < 0) {
      GetErrorLogInstance()
          << "Failed to get a file descriptor for " << name << "\n";
      return NoCounters();
    }

    counter_ids[i] = id;
  }
  if (ioctl(counter_ids[0], PERF_EVENT_IOC_ENABLE) != 0) {
    GetErrorLogInstance() << "Failed to start counters\n";
    return NoCounters();
  }

  return PerfCounters(counter_names, std::move(counter_ids));
}

void PerfCounters::CloseCounters() const {
  if (counter_ids_.empty()) {
    return;
  }
  ioctl(counter_ids_[0], PERF_EVENT_IOC_DISABLE);
  for (int fd : counter_ids_) {
    close(fd);
  }
}
#else   // defined HAVE_LIBPFM
const bool PerfCounters::kSupported = false;

bool PerfCounters::Initialize() { return false; }

PerfCounters PerfCounters::Create(
    const std::vector<std::string>& counter_names) {
  if (!counter_names.empty()) {
    GetErrorLogInstance() << "Performance counters not supported.";
  }
  return NoCounters();
}

void PerfCounters::CloseCounters() const {}
#endif  // defined HAVE_LIBPFM

Mutex PerfCountersMeasurement::mutex_;
int PerfCountersMeasurement::ref_count_ = 0;
PerfCounters PerfCountersMeasurement::counters_ = PerfCounters::NoCounters();

PerfCountersMeasurement::PerfCountersMeasurement(
    const std::vector<std::string>& counter_names)
    : start_values_(counter_names.size()), end_values_(counter_names.size()) {
  MutexLock l(mutex_);
  if (ref_count_ == 0) {
    counters_ = PerfCounters::Create(counter_names);
  }
  // We chose to increment it even if `counters_` ends up invalid,
  // so that we don't keep trying to create, and also since the dtor
  // will decrement regardless of `counters_`'s validity
  ++ref_count_;

  BM_CHECK(!counters_.IsValid() || counters_.names() == counter_names);
}

PerfCountersMeasurement::~PerfCountersMeasurement() {
  MutexLock l(mutex_);
  --ref_count_;
  if (ref_count_ == 0) {
    counters_ = PerfCounters::NoCounters();
  }
}

PerfCounters& PerfCounters::operator=(PerfCounters&& other) noexcept {
  if (this != &other) {
    CloseCounters();

    counter_ids_ = std::move(other.counter_ids_);
    counter_names_ = std::move(other.counter_names_);
  }
  return *this;
}
}  // namespace internal
}  // namespace benchmark
