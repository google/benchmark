
#undef NDEBUG
#include <utility>

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
// ------------------------- Basic Counters Output ------------------------- //
// ========================================================================= //

void BM_Counters_Simple(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.counters["foo"] = 1;
  state.counters["bar"] = 2;
  //state.SetItemsProcessed(150);
  //state.SetBytesProcessed(364);
}
BENCHMARK(BM_Counters_Simple);//->ThreadRange(1, 32);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_Simple %console_report bar=%float foo=%float$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_Simple\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"bar\": %float,$", MR_Next},
                       {"\"foo\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_Counters_Simple\",%csv_report,%float,%float$"}});

// ========================================================================= //
// ------------------------- Basic Counters Output ------------------------- //
// ========================================================================= //

void BM_Counters_WithBytesAndItemsPSec(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.counters["foo"] = 1;
  state.counters["bar"] = 2;
  state.SetItemsProcessed(150);
  state.SetBytesProcessed(364);
}
BENCHMARK(BM_Counters_WithBytesAndItemsPSec);//->ThreadRange(1, 32);
ADD_CASES(TC_ConsoleOut,
          {{"^BM_Counters_WithBytesAndItemsPSec %console_report "
            "bar=%float foo=%float +%floatB/s +%float items/s$"}});
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

// ========================================================================= //
// --------------------------- TEST CASES END ------------------------------ //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
