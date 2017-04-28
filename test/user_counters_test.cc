
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
ADD_CASES(TC_CSVOut, {{"%csv_header,\"bar\",\"foo\""}});

// ========================================================================= //
// ------------------------- Simple Counters Output ------------------------ //
// ========================================================================= //

void BM_Counters_Simple(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.counters["foo"] = 1;
  state.counters["bar"] = 2;
}
BENCHMARK(BM_Counters_Simple);//->ThreadRange(1, 32);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_Simple %console_report bar=%hrfloat foo=%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_Simple\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_Counters_Simple\",%csv_report,%float,%float$"}});
CHECK_BENCHMARK_RESULTS("BM_Counters_Simple", [](ResultsCheckerEntry const& e) {
  CHECK_COUNTER_VALUE(e, int, "foo", EQ, 1);
  CHECK_COUNTER_VALUE(e, int, "bar", EQ, 2);
});

// ========================================================================= //
// --------------------- Counters+Items+Bytes/s Output --------------------- //
// ========================================================================= //

namespace { int num_calls1 = 0; }
void BM_Counters_WithBytesAndItemsPSec(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.counters["foo"] = 1;
  state.counters["bar"] = ++num_calls1;
  state.SetBytesProcessed(364);
  state.SetItemsProcessed(150);
}
BENCHMARK(BM_Counters_WithBytesAndItemsPSec);//->ThreadRange(1, 32);
ADD_CASES(TC_ConsoleOut,
          {{"^BM_Counters_WithBytesAndItemsPSec %console_report "
            "bar=%hrfloat foo=%hrfloat +%floatB/s +%float items/s$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_WithBytesAndItemsPSec\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bytes_per_second\": %int,$", MR_Next},
                       {"\"items_per_second\": %int,$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_Counters_WithBytesAndItemsPSec\","
                       "%csv_bytes_items_report,%float,%float$"}});
CHECK_BENCHMARK_RESULTS("BM_Counters_WithBytesAndItemsPSec",
                        [](ResultsCheckerEntry const& e) {
  CHECK_COUNTER_VALUE(e, int, "foo", EQ, 1);
  CHECK_COUNTER_VALUE(e, int, "bar", EQ, num_calls1);
  //TODO CHECK_RESULT_VALUE(e, int, "bytes_per_second", EQ, 364 / e.duration_cpu_time());
  //TODO CHECK_RESULT_VALUE(e, int, "items_per_second", EQ, 150 / e.duration_cpu_time());
});

// ========================================================================= //
// ------------------------- Rate Counters Output -------------------------- //
// ========================================================================= //

void BM_Counters_Rate(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  namespace bm = benchmark;
  state.counters["foo"] = bm::Counter{1, bm::Counter::kIsRate};
  state.counters["bar"] = bm::Counter{2, bm::Counter::kIsRate};
}
BENCHMARK(BM_Counters_Rate);//->ThreadRange(1, 32);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_Rate %console_report bar=%hrfloat foo=%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_Rate\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_Counters_Rate\",%csv_report,%float,%float$"}});

// ========================================================================= //
// ------------------------- Thread Counters Output ------------------------ //
// ========================================================================= //

void BM_Counters_Threads(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.counters["foo"] = 1;
  state.counters["bar"] = 2;
}
BENCHMARK(BM_Counters_Threads)->ThreadRange(1, 8);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_Threads/threads:%int %console_report bar=%hrfloat foo=%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_Threads/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_Counters_Threads/threads:%int\",%csv_report,%float,%float$"}});

// ========================================================================= //
// ---------------------- ThreadAvg Counters Output ------------------------ //
// ========================================================================= //

void BM_Counters_AvgThreads(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  namespace bm = benchmark;
  state.counters["foo"] = bm::Counter{1, bm::Counter::kAvgThreads};
  state.counters["bar"] = bm::Counter{2, bm::Counter::kAvgThreads};
}
BENCHMARK(BM_Counters_AvgThreads)->ThreadRange(1, 8);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_AvgThreads/threads:%int %console_report bar=%hrfloat foo=%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_AvgThreads/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_Counters_AvgThreads/threads:%int\",%csv_report,%float,%float$"}});

// ========================================================================= //
// ---------------------- ThreadAvg Counters Output ------------------------ //
// ========================================================================= //

void BM_Counters_AvgThreadsRate(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  namespace bm = benchmark;
  state.counters["foo"] = bm::Counter{1, bm::Counter::kAvgThreadsRate};
  state.counters["bar"] = bm::Counter{2, bm::Counter::kAvgThreadsRate};
}
BENCHMARK(BM_Counters_AvgThreadsRate)->ThreadRange(1, 8);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_AvgThreadsRate/threads:%int %console_report bar=%hrfloat foo=%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_AvgThreadsRate/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_Counters_AvgThreadsRate/threads:%int\",%csv_report,%float,%float$"}});

// ========================================================================= //
// --------------------------- TEST CASES END ------------------------------ //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
