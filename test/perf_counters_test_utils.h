#ifndef PERF_COUNTERS_TEST_UTILS_H_
#define PERF_COUNTERS_TEST_UTILS_H_

#include <set>
#include <string>
#include <vector>

#include "../src/perf_counters.h"

namespace benchmark {
namespace internal {
namespace test {

inline std::set<std::string> UniqueCounterNames(const PerfCounters& counters) {
  return {counters.names().begin(), counters.names().end()};
}

inline bool HasRequiredPerfCounters(const std::vector<std::string>& names) {
  if (!PerfCounters::kSupported) {
    return false;
  }
  auto counters = PerfCounters::Create(names);
  auto actual_names = UniqueCounterNames(counters);
  for (const auto& name : names) {
    if (actual_names.find(name) == actual_names.end()) {
      return false;
    }
  }
  return true;
}

}  // namespace test
}  // namespace internal
}  // namespace benchmark

#endif  // PERF_COUNTERS_TEST_UTILS_H_
