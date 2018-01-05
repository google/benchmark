
#undef NDEBUG

#include "benchmark/benchmark.h"
#include "output_test.h"

// ========================================================================= //
// ---------------------- Testing Prologue Output -------------------------- //
// ========================================================================= //

ADD_CASES(TC_ConsoleOut,
          {{"^[-]+$", MR_Next},
           {"^Benchmark %s Time %s CPU %s Iterations UserCounters...$", MR_Next},
           {"^[-]+$", MR_Next}});

// ========================================================================= //
// ------------------------- Simple Counters Output ------------------------ //
// ========================================================================= //

void BM_Counters_Simple(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.counters["foo"] = 1;
  state.counters["bar"] = 2 * (double)state.iterations();
}
BENCHMARK(BM_Counters_Simple);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_Simple %console_report bar=%hrfloat foo=%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_Simple\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// --------------------- Counters+Items+Bytes/s Output --------------------- //
// ========================================================================= //

namespace { int num_calls1 = 0; }
void BM_Counters_WithBytesAndItemsPSec(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.counters["foo"] = 1;
  state.counters["bar"] = ++num_calls1;
  state.SetBytesProcessed(364);
  state.SetItemsProcessed(150);
}
BENCHMARK(BM_Counters_WithBytesAndItemsPSec);
ADD_CASES(TC_ConsoleOut,
          {{"^BM_Counters_WithBytesAndItemsPSec %console_report "
            "bar=%hrfloat foo=%hrfloat +%hrfloatB/s +%hrfloat items/s$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_WithBytesAndItemsPSec\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bytes_per_second\": %float,$", MR_Next},
                       {"\"items_per_second\": %float,$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// ------------------------- Rate Counters Output -------------------------- //
// ========================================================================= //

void BM_Counters_Rate(benchmark::State& state) {
  for (auto _ : state) {
  }
  namespace bm = benchmark;
  state.counters["foo"] = bm::Counter{1, bm::Counter::kIsRate};
  state.counters["bar"] = bm::Counter{2, bm::Counter::kIsRate};
}
BENCHMARK(BM_Counters_Rate);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_Rate %console_report bar=%hrfloat/s foo=%hrfloat/s$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_Rate\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// ------------------------- Thread Counters Output ------------------------ //
// ========================================================================= //

void BM_Counters_Threads(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.counters["foo"] = 1;
  state.counters["bar"] = 2;
}
BENCHMARK(BM_Counters_Threads)->ThreadRange(1, 8);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_Threads/threads:%int %console_report bar=%hrfloat foo=%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_Threads/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// ---------------------- ThreadAvg Counters Output ------------------------ //
// ========================================================================= //

void BM_Counters_AvgThreads(benchmark::State& state) {
  for (auto _ : state) {
  }
  namespace bm = benchmark;
  state.counters["foo"] = bm::Counter{1, bm::Counter::kAvgThreads};
  state.counters["bar"] = bm::Counter{2, bm::Counter::kAvgThreads};
}
BENCHMARK(BM_Counters_AvgThreads)->ThreadRange(1, 8);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_AvgThreads/threads:%int %console_report bar=%hrfloat foo=%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_AvgThreads/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// ---------------------- ThreadAvg Counters Output ------------------------ //
// ========================================================================= //

void BM_Counters_AvgThreadsRate(benchmark::State& state) {
  for (auto _ : state) {
  }
  namespace bm = benchmark;
  state.counters["foo"] = bm::Counter{1, bm::Counter::kAvgThreadsRate};
  state.counters["bar"] = bm::Counter{2, bm::Counter::kAvgThreadsRate};
}
BENCHMARK(BM_Counters_AvgThreadsRate)->ThreadRange(1, 8);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_AvgThreadsRate/threads:%int %console_report bar=%hrfloat/s foo=%hrfloat/s$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_AvgThreadsRate/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// --------------------------- TEST CASES END ------------------------------ //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
