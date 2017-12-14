
#undef NDEBUG

#include "benchmark/benchmark.h"
#include "output_test.h"

// @todo: <jpmag> this checks the full output at once; the rule for
// CounterSet1 was failing because it was not matching "^[-]+$".
// @todo: <jpmag> check that the counters are vertically aligned.
ADD_CASES(TC_ConsoleOut, {
// keeping these lines long improves readability, so:
// clang-format off
    {"^[-]+$", MR_Next},
    {"^Benchmark %s Time %s CPU %s Iterations %s Bar %s Bat %s Baz %s Foo %s Frob %s Lob$", MR_Next},
    {"^[-]+$", MR_Next},
    {"^BM_Counters_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_Counters_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_Counters_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_Counters_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_Counters_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterRates_Tabular/threads:%int %console_report [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s$", MR_Next},
    {"^BM_CounterRates_Tabular/threads:%int %console_report [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s$", MR_Next},
    {"^BM_CounterRates_Tabular/threads:%int %console_report [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s$", MR_Next},
    {"^BM_CounterRates_Tabular/threads:%int %console_report [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s$", MR_Next},
    {"^BM_CounterRates_Tabular/threads:%int %console_report [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s$", MR_Next},
    {"^[-]+$", MR_Next},
    {"^Benchmark %s Time %s CPU %s Iterations %s Bar %s Baz %s Foo$", MR_Next},
    {"^[-]+$", MR_Next},
    {"^BM_CounterSet0_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet0_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet0_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet0_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet0_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet1_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet1_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet1_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet1_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet1_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^[-]+$", MR_Next},
    {"^Benchmark %s Time %s CPU %s Iterations %s Bat %s Baz %s Foo$", MR_Next},
    {"^[-]+$", MR_Next},
    {"^BM_CounterSet2_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet2_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet2_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet2_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$", MR_Next},
    {"^BM_CounterSet2_Tabular/threads:%int %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$"},
// clang-format on
});

// ========================================================================= //
// ------------------------- Tabular Counters Output ----------------------- //
// ========================================================================= //

void BM_Counters_Tabular(benchmark::State& state) {
  for (auto _ : state) {
  }
  namespace bm = benchmark;
  state.counters.insert({
    {"Foo",  { 1, bm::Counter::kAvgThreads}},
    {"Bar",  { 2, bm::Counter::kAvgThreads}},
    {"Baz",  { 4, bm::Counter::kAvgThreads}},
    {"Bat",  { 8, bm::Counter::kAvgThreads}},
    {"Frob", {16, bm::Counter::kAvgThreads}},
    {"Lob",  {32, bm::Counter::kAvgThreads}},
  });
}
BENCHMARK(BM_Counters_Tabular)->ThreadRange(1, 16);
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_Tabular/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"Bar\": %float,$", MR_Next},
                       {"\"Bat\": %float,$", MR_Next},
                       {"\"Baz\": %float,$", MR_Next},
                       {"\"Foo\": %float,$", MR_Next},
                       {"\"Frob\": %float,$", MR_Next},
                       {"\"Lob\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// -------------------- Tabular+Rate Counters Output ----------------------- //
// ========================================================================= //

void BM_CounterRates_Tabular(benchmark::State& state) {
  for (auto _ : state) {
  }
  namespace bm = benchmark;
  state.counters.insert({
    {"Foo",  { 1, bm::Counter::kAvgThreadsRate}},
    {"Bar",  { 2, bm::Counter::kAvgThreadsRate}},
    {"Baz",  { 4, bm::Counter::kAvgThreadsRate}},
    {"Bat",  { 8, bm::Counter::kAvgThreadsRate}},
    {"Frob", {16, bm::Counter::kAvgThreadsRate}},
    {"Lob",  {32, bm::Counter::kAvgThreadsRate}},
  });
}
BENCHMARK(BM_CounterRates_Tabular)->ThreadRange(1, 16);
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_CounterRates_Tabular/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"Bar\": %float,$", MR_Next},
                       {"\"Bat\": %float,$", MR_Next},
                       {"\"Baz\": %float,$", MR_Next},
                       {"\"Foo\": %float,$", MR_Next},
                       {"\"Frob\": %float,$", MR_Next},
                       {"\"Lob\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// ------------------------- Tabular Counters Output ----------------------- //
// ========================================================================= //

// set only some of the counters
void BM_CounterSet0_Tabular(benchmark::State& state) {
  for (auto _ : state) {
  }
  namespace bm = benchmark;
  state.counters.insert({
    {"Foo", {10, bm::Counter::kAvgThreads}},
    {"Bar", {20, bm::Counter::kAvgThreads}},
    {"Baz", {40, bm::Counter::kAvgThreads}},
  });
}
BENCHMARK(BM_CounterSet0_Tabular)->ThreadRange(1, 16);
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_CounterSet0_Tabular/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"Bar\": %float,$", MR_Next},
                       {"\"Baz\": %float,$", MR_Next},
                       {"\"Foo\": %float$", MR_Next},
                       {"}", MR_Next}});

// again.
void BM_CounterSet1_Tabular(benchmark::State& state) {
  for (auto _ : state) {
  }
  namespace bm = benchmark;
  state.counters.insert({
    {"Foo", {15, bm::Counter::kAvgThreads}},
    {"Bar", {25, bm::Counter::kAvgThreads}},
    {"Baz", {45, bm::Counter::kAvgThreads}},
  });
}
BENCHMARK(BM_CounterSet1_Tabular)->ThreadRange(1, 16);
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_CounterSet1_Tabular/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"Bar\": %float,$", MR_Next},
                       {"\"Baz\": %float,$", MR_Next},
                       {"\"Foo\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// ------------------------- Tabular Counters Output ----------------------- //
// ========================================================================= //

// set only some of the counters, different set now.
void BM_CounterSet2_Tabular(benchmark::State& state) {
  for (auto _ : state) {
  }
  namespace bm = benchmark;
  state.counters.insert({
    {"Foo", {10, bm::Counter::kAvgThreads}},
    {"Bat", {30, bm::Counter::kAvgThreads}},
    {"Baz", {40, bm::Counter::kAvgThreads}},
  });
}
BENCHMARK(BM_CounterSet2_Tabular)->ThreadRange(1, 16);
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_CounterSet2_Tabular/threads:%int\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"Bat\": %float,$", MR_Next},
                       {"\"Baz\": %float,$", MR_Next},
                       {"\"Foo\": %float$", MR_Next},
                       {"}", MR_Next}});

// ========================================================================= //
// --------------------------- TEST CASES END ------------------------------ //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
