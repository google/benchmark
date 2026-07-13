// Verifies that a registered MemoryManager only brackets the benchmark's
// measured work: the library's own ThreadManager allocation and the
// per-benchmark Setup()/Teardown() must run outside the Start()/Stop()
// window, consistent with how they run outside the timed region. See
// https://github.com/google/benchmark/issues/2149.

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"

namespace {

// Ordered log of lifecycle events. All of these callbacks run on the main
// thread (thread_index 0), so no synchronization is needed.
std::vector<std::string> events;

class OrderingMemoryManager : public benchmark::MemoryManager {
 public:
  void Start() override { events.emplace_back("Start"); }
  void Stop(Result& result) override {
    events.emplace_back("Stop");
    result.num_allocs = 0;
    result.max_bytes_used = 0;
  }
};

void DoSetup(const benchmark::State&) { events.emplace_back("Setup"); }
void DoTeardown(const benchmark::State&) { events.emplace_back("Teardown"); }

void BM_ordering(benchmark::State& state) {
  for (auto _ : state) {
  }
}
BENCHMARK(BM_ordering)->Iterations(1)->Setup(DoSetup)->Teardown(DoTeardown);

}  // namespace

int main(int argc, char** argv) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);

  OrderingMemoryManager mm;
  benchmark::RegisterMemoryManager(&mm);

  benchmark::Initialize(&argc, argv);
  const size_t ret = benchmark::RunSpecifiedBenchmarks(".");
  assert(ret > 0);

  benchmark::RegisterMemoryManager(nullptr);

  // Exactly one memory-measurement pass should have run.
  auto start = std::find(events.begin(), events.end(), "Start");
  auto stop = std::find(events.begin(), events.end(), "Stop");
  assert(start != events.end() && "MemoryManager::Start was never called");
  assert(stop != events.end() && "MemoryManager::Stop was never called");
  assert(start < stop && "Start must precede Stop");

  // Nothing between Start and Stop may be a Setup or Teardown: those run
  // outside the measured window (this is what #2149 fixes).
  for (auto it = start + 1; it != stop; ++it) {
    assert(*it != "Setup" &&
           "Setup ran inside the MemoryManager Start/Stop window");
    assert(*it != "Teardown" &&
           "Teardown ran inside the MemoryManager Start/Stop window");
  }

  return 0;
}
