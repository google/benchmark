
#undef NDEBUG

#include "benchmark/benchmark.h"
#include "output_test.h"

// ========================================================================= //
// ---------------------- Testing Prologue Output -------------------------- //
// ========================================================================= //

ADD_CASES(TC_ConsoleOut,
          {{"^[-]+$", MR_Next},
           {"^Benchmark %s Time %s CPU %s Iterations UserCounters... MetaData...$", MR_Next},
           {"^[-]+$", MR_Next}});
ADD_CASES(TC_CSVOut, {{"%csv_header,\"bar\",\"foo\",some_key_foo"}});

// ========================================================================= //
// ------------------------- Simple Counters Output ------------------------ //
// ========================================================================= //

void BM_Meta_Empty(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.counters["foo"] = 1;
  state.counters["bar"] = 2 * (double)state.iterations();
}
BENCHMARK(BM_Meta_Empty)
->AddMetaData("some_key_foo","some_val_foo")
;
// ADD_CASES(TC_ConsoleOut, {{
//     "^BM_Meta_Empty %console_report bar=%hrfloat foo=%hrfloat some_key_foo=some_val_foo$"
// }});
// ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Meta_Empty\",$"},
//                        {"\"iterations\": %float,$", MR_Next},
//                        {"\"real_time\": %float,$", MR_Next},
//                        {"\"cpu_time\": %float,$", MR_Next},
//                        {"\"time_unit\": \"ns\",$", MR_Next},
//                        {"\"bar\": %float,$", MR_Next},
//                        {"\"foo\": %float$", MR_Next},
//                        {"}", MR_Next}});
// ADD_CASES(TC_CSVOut, {{"^\"BM_Meta_Empty\",%csv_report,%float,%float$"}});
// // VS2013 does not allow this function to be passed as a lambda argument
// // to CHECK_BENCHMARK_RESULTS()
// void CheckSimple(Results const& e) {
//   double its = e.GetAs< double >("iterations");
//   CHECK_COUNTER_VALUE(e, int, "foo", EQ, 1);
//   // check that the value of bar is within 0.1% of the expected value
//   CHECK_FLOAT_COUNTER_VALUE(e, "bar", EQ, 2.*its, 0.001);
// }
// CHECK_BENCHMARK_RESULTS("BM_Counters_Simple", &CheckSimple);




// ========================================================================= //
// --------------------------- TEST CASES END ------------------------------ //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
