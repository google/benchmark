#include "benchmark_api_internal.h"

#include <memory>

namespace benchmark {
namespace internal {

std::unique_ptr<State> BenchmarkInstance::Run(
    size_t iters, int thread_id, internal::ThreadTimer* timer,
    internal::ThreadManager* manager) const {
  auto st = std::unique_ptr<State>(new State(iters, arg, thread_id, threads,
                                             timer, manager));
  benchmark->Run(*st);
  return st;
}

}  // internal
}  // benchmark
