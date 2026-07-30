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

#ifndef BENCHMARK_PERF_COUNTERS_LIBPFM_H
#define BENCHMARK_PERF_COUNTERS_LIBPFM_H

#include <linux/perf_event.h>

#include "perfmon/pfmlib.h"
#include "perfmon/pfmlib_perf_event.h"

namespace benchmark {
namespace internal {

inline int ConfigurePerfEventAttr(const char* name, bool is_group_leader,
                                  perf_event_attr* attr) {
  *attr = {};
  attr->size = sizeof(*attr);

  pfm_perf_encode_arg_t arg{};
  arg.attr = attr;
  const int kCounterMode = PFM_PLM3;  // user mode unless overridden by name
  const int status =
      pfm_get_os_event_encoding(name, kCounterMode, PFM_OS_PERF_EVENT, &arg);
  if (status != PFM_SUCCESS) {
    return status;
  }

  // Preserve the privilege exclusions encoded by libpfm for modifiers such as
  // ":u" and ":k", and populate only the benchmark-owned group attributes.
  attr->disabled = is_group_leader;
  attr->inherit = true;
  attr->pinned = is_group_leader;
  attr->read_format = PERF_FORMAT_GROUP;
  return PFM_SUCCESS;
}

}  // namespace internal
}  // namespace benchmark

#endif  // BENCHMARK_PERF_COUNTERS_LIBPFM_H
