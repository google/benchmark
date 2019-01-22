#ifndef BENCHMARK_PERFORMANCE_COUNTER_H
#define BENCHMARK_PERFORMANCE_COUNTER_H

#include "benchmark/benchmark.h"
#include <vector>
#include <string>
#include <ostream>

namespace benchmark {
namespace internal {

using PerformanceEvents = std::vector<int>;

class PerformanceCounter
{
public:
  explicit PerformanceCounter(const PerformanceEvents& events);
  ~PerformanceCounter();

  bool Start();
  bool Stop();

  void IncrementCounters(UserCounters&) const;

  static PerformanceEvents ReadEvents(const std::string& input, std::ostream& err_stream);

private:
#ifdef BENCHMARK_HAS_PAPI
  int event_set_;
  std::vector<long long> counters_;
  std::vector<std::string> event_names_;
#endif
};

}  // namespace internal
}  // namespace benchmark

#endif  // BENCHMARK_PERFORMANCE_COUNTER_H
