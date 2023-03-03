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
#include "perfmon/pfmlib.h"
#include "perfmon/pfmlib_perf_event.h"
#endif

namespace benchmark {
namespace internal {

constexpr size_t PerfCounterValues::kMaxCounters;

#if defined HAVE_LIBPFM

size_t PerfCounterValues::Read(const std::vector<int>& leaders) {
  // Create a pointer for multiple reads
  const size_t bufsize = values_.size() * sizeof(values_[0]);
  char* ptr = reinterpret_cast<char*>(values_.data());
  size_t size = bufsize;
  for (int lead : leaders) {
    auto read_bytes = ::read(lead, ptr, size);
    if (read_bytes >= ssize_t(sizeof(uint64_t))) {
      // Actual data bytes are all bytes minus initial padding
      std::size_t data_bytes = read_bytes - sizeof(uint64_t);
      // This should be very cheap since it's in hot cache
      std::memmove(ptr, ptr + sizeof(uint64_t), data_bytes);
      // Increment our counters
      ptr += data_bytes;
      size -= data_bytes;
    } else {
      int err = errno;
      GetErrorLogInstance()
          << "PerfCounterValues: error reading fd:" << lead << " errno:" << err
          << " " << ::strerror(err) << "\n";
      return 0;
    }
  }
  return (bufsize - size) / sizeof(uint64_t);
}

const bool PerfCounters::kSupported = true;

bool PerfCounters::Initialize() { return pfm_initialize() == PFM_SUCCESS; }

bool PerfCounters::IsCounterSupported(const std::string& name) {
  perf_event_attr_t attr;
  std::memset(&attr, 0, sizeof(attr));
  pfm_perf_encode_arg_t arg;
  std::memset(&arg, 0, sizeof(arg));
  arg.attr = &attr;
  const int mode = PFM_PLM3;  // user mode only
  int ret = pfm_get_os_event_encoding(name.c_str(), mode, PFM_OS_PERF_EVENT_EXT,
                                      &arg);
  return (ret == PFM_SUCCESS);
}

PerfCounters PerfCounters::Create(
    const std::vector<std::string>& counter_names) {
  if (counter_names.empty()) {
    return NoCounters();
  }
  if (counter_names.size() > PerfCounterValues::kMaxCounters) {
    GetErrorLogInstance()
        << counter_names.size()
        << " counters were requested. The minimum is 1, the maximum is "
        << PerfCounterValues::kMaxCounters << "\n";
    return NoCounters();
  }
  std::vector<int> counter_ids(counter_names.size());
  std::vector<int> leader_ids;

  const int mode = PFM_PLM3;  // user mode only
  int group_id = -1;
  for (size_t i = 0; i < counter_names.size(); ++i) {
    const bool is_first = (group_id < 0);
    struct perf_event_attr attr {};
    attr.size = sizeof(attr);
    const auto& name = counter_names[i];
    if (name.empty()) {
      GetErrorLogInstance() << "A counter name was the empty string\n";
      return NoCounters();
    }
    pfm_perf_encode_arg_t arg{};
    arg.attr = &attr;

    const int pfm_get =
        pfm_get_os_event_encoding(name.c_str(), mode, PFM_OS_PERF_EVENT, &arg);
    if (pfm_get != PFM_SUCCESS) {
      GetErrorLogInstance() << "Unknown counter name: " << name << "\n";
      return NoCounters();
    }
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
    while (id < 0) {
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
        // We reached a limit perhaps?
        if (group_id >= 0) {
          // Create a new group
          group_id = -1;
        } else {
          // Give up, there is nothing else to try
          break;
        }
      }
    }
    if (id < 0) {
      GetErrorLogInstance()
          << "Failed to get a file descriptor for " << name << "\n";
      return NoCounters();
    }
    if (group_id < 0) {
      // This is a leader, store and assign it
      leader_ids.push_back(id);
      group_id = id;
    }
    counter_ids[i] = id;
  }
  for (int lead : leader_ids) {
    if (ioctl(lead, PERF_EVENT_IOC_ENABLE) != 0) {
      GetErrorLogInstance() << "Failed to start counters\n";
      return NoCounters();
    }
  }

  return PerfCounters(counter_names, std::move(counter_ids),
                      std::move(leader_ids));
}

void PerfCounters::CloseCounters() const {
  if (counter_ids_.empty()) {
    return;
  }
  for (int lead : leader_ids_) {
    ioctl(lead, PERF_EVENT_IOC_DISABLE);
  }
  for (int fd : counter_ids_) {
    close(fd);
  }
}
#else   // defined HAVE_LIBPFM
size_t PerfCounterValues::Read(const std::vector<int>&) { return 0; }

const bool PerfCounters::kSupported = false;

bool PerfCounters::Initialize() { return false; }

bool PerfCounters::IsCounterSupported(const std::string&) { return false; }

PerfCounters PerfCounters::Create(
    const std::vector<std::string>& counter_names) {
  if (!counter_names.empty()) {
    GetErrorLogInstance() << "Performance counters not supported.";
  }
  return NoCounters();
}

void PerfCounters::CloseCounters() const {}
#endif  // defined HAVE_LIBPFM

PerfCountersMeasurement::PerfCountersMeasurement(
    const std::vector<std::string>& counter_names)
    : start_values_(counter_names.size()), end_values_(counter_names.size()) {
  counters_ = PerfCounters::Create(counter_names);
}

PerfCountersMeasurement::~PerfCountersMeasurement() {}

PerfCounters& PerfCounters::operator=(PerfCounters&& other) noexcept {
  if (this != &other) {
    CloseCounters();

    counter_ids_ = std::move(other.counter_ids_);
    leader_ids_ = std::move(other.leader_ids_);
    counter_names_ = std::move(other.counter_names_);
  }
  return *this;
}
}  // namespace internal
}  // namespace benchmark
