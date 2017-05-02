
#undef NDEBUG

#include "benchmark/benchmark.h"
#include "output_test.h"

// ========================================================================= //
// ---------------------- Testing Prologue Output -------------------------- //
// ========================================================================= //

ADD_CASES(TC_ConsoleOut,
          {{"^[-]+$", MR_Next},
           {"^Benchmark %s Time %s CPU %s Iterations %s Bar %s Bat %s Baz %s Foo %s Frob %s Lob$",
            MR_Next},
           {"^[-]+$", MR_Next}});
ADD_CASES(TC_CSVOut, {{"%csv_header,\"Bar\",\"Bat\",\"Baz\",\"Foo\",\"Frob\",\"Lob\""}});

// ========================================================================= //
// ------------------------- Tabular Counters Output ----------------------- //
// ========================================================================= //

void BM_Counters_Tabular(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.counters.insert({
    {"Foo", 1},
    {"Bar", 2},
    {"Baz", 4},
    {"Bat", 8},
    {"Frob", 16},
    {"Lob", 32},
  });
}
BENCHMARK(BM_Counters_Tabular);
ADD_CASES(TC_ConsoleOut, {{"^BM_Counters_Tabular %console_report "
                           "[ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat "
                           "[ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Counters_Tabular\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"Bar\": %float,$", MR_Next},
                       {"\"Bat\": %float,$", MR_Next},
                       {"\"Baz\": %float,$", MR_Next},
                       {"\"Foo\": %float,$", MR_Next},
                       {"\"Frob\": %float,$", MR_Next},
                       {"\"Lob\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_Counters_Tabular\",%csv_report,"
                       "%float,%float,%float,%float,%float,%float$"}});
// VS2013 does not allow this function to be passed as a lambda argument
// to CHECK_BENCHMARK_RESULTS()
void CheckTabular(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "Foo", EQ, 1);
  CHECK_COUNTER_VALUE(e, int, "Bar", EQ, 2);
  CHECK_COUNTER_VALUE(e, int, "Baz", EQ, 4);
  CHECK_COUNTER_VALUE(e, int, "Bat", EQ, 8);
  CHECK_COUNTER_VALUE(e, int, "Frob", EQ, 16);
  CHECK_COUNTER_VALUE(e, int, "Lob", EQ, 32);
}
CHECK_BENCHMARK_RESULTS("BM_Counters_Tabular", &CheckTabular);

// ========================================================================= //
// -------------------- Tabular+Rate Counters Output ----------------------- //
// ========================================================================= //

void BM_CounterRates_Tabular(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  namespace bm = benchmark;
  state.counters.insert({
    {"Foo",  { 1, bm::Counter::kIsRate}},
    {"Bar",  { 2, bm::Counter::kIsRate}},
    {"Baz",  { 4, bm::Counter::kIsRate}},
    {"Bat",  { 8, bm::Counter::kIsRate}},
    {"Frob", {16, bm::Counter::kIsRate}},
    {"Lob",  {32, bm::Counter::kIsRate}},
  });
}
BENCHMARK(BM_CounterRates_Tabular);
ADD_CASES(TC_ConsoleOut, {{"^BM_CounterRates_Tabular %console_report "
                           "[ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s "
                           "[ ]*%hrfloat/s [ ]*%hrfloat/s [ ]*%hrfloat/s$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_CounterRates_Tabular\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"Bar\": %float,$", MR_Next},
                       {"\"Bat\": %float,$", MR_Next},
                       {"\"Baz\": %float,$", MR_Next},
                       {"\"Foo\": %float,$", MR_Next},
                       {"\"Frob\": %float,$", MR_Next},
                       {"\"Lob\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_CounterRates_Tabular\",%csv_report,"
                       "%float,%float,%float,%float,%float,%float$"}});
// VS2013 does not allow this function to be passed as a lambda argument
// to CHECK_BENCHMARK_RESULTS()
void CheckTabularRate(Results const& e) {
  double t = e.DurationCPUTime();
  CHECK_FLOAT_COUNTER_VALUE(e, "Foo", EQ, 1./t, 0.001);
  CHECK_FLOAT_COUNTER_VALUE(e, "Bar", EQ, 2./t, 0.001);
  CHECK_FLOAT_COUNTER_VALUE(e, "Baz", EQ, 4./t, 0.001);
  CHECK_FLOAT_COUNTER_VALUE(e, "Bat", EQ, 8./t, 0.001);
  CHECK_FLOAT_COUNTER_VALUE(e, "Frob", EQ, 16./t, 0.001);
  CHECK_FLOAT_COUNTER_VALUE(e, "Lob", EQ, 32./t, 0.001);
}
CHECK_BENCHMARK_RESULTS("BM_CounterRates_Tabular", &CheckTabularRate);

// ========================================================================= //
// ------------------------- Tabular Counters Output ----------------------- //
// ========================================================================= //

// set only some of the counters
void BM_CounterSet_Tabular(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.counters.insert({
    {"Foo", 10},
    {"Bar", 20},
    {"Baz", 40},
  });
}
BENCHMARK(BM_CounterSet_Tabular);
ADD_CASES(TC_ConsoleOut, {
    {"^[-]+$", MR_Next},
    {"^Benchmark %s Time %s CPU %s Iterations %s Bar %s Baz %s Foo$", MR_Next},
    {"^[-]+$", MR_Next},
    {"^BM_CounterSet_Tabular %console_report [ ]*%hrfloat [ ]*%hrfloat [ ]*%hrfloat$"}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_CounterSet_Tabular\",$"},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %int,$", MR_Next},
                       {"\"cpu_time\": %int,$", MR_Next},
                       {"\"time_unit\": \"ns\",$", MR_Next},
                       {"\"Bar\": %float,$", MR_Next},
                       {"\"Baz\": %float,$", MR_Next},
                       {"\"Foo\": %float$", MR_Next},
                       {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_CounterSet_Tabular\",%csv_report,"
                       "%float,,%float,%float,,"}});
// VS2013 does not allow this function to be passed as a lambda argument
// to CHECK_BENCHMARK_RESULTS()
void CheckSet(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "Foo", EQ, 10);
  CHECK_COUNTER_VALUE(e, int, "Bar", EQ, 20);
  CHECK_COUNTER_VALUE(e, int, "Baz", EQ, 40);
}
CHECK_BENCHMARK_RESULTS("BM_CounterSet_Tabular", &CheckSet);

// ========================================================================= //
// --------------------------- TEST CASES END ------------------------------ //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
