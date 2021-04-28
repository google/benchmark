#include "benchmark_api_internal.h"

namespace benchmark {
namespace internal {

State BenchmarkInstance::Run(
    IterationCount iters, int thread_id, internal::ThreadTimer* timer,
    internal::ThreadManager* manager,
    internal::PerfCountersMeasurement* perf_counters_measurement) const {
  State st(iters, arg, thread_id, threads, timer, manager,
           perf_counters_measurement);
  benchmark->Run(st);
  return st;
}

}  // internal
}  // benchmark
