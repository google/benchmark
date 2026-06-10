#undef NDEBUG

#include "benchmark/benchmark_api.h"
#include "benchmark/registration.h"
#include "benchmark/state.h"
#include "benchmark/utils.h"
#include "output_test.h"

// clang-format off

ADD_CASES(TC_ConsoleOut,
          {{"^[-]+$", MR_Next},
           {"^Benchmark %s Time %s CPU %s Iterations UserCounters...$", MR_Next},
           {"^[-]+$", MR_Next}});
ADD_CASES(TC_CSVOut,
          {{"%csv_header,\"work\""}});

// clang-format on

namespace {
void BM_ThreadCounterStats(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.counters["work"] = state.thread_index() + 1;
}
BENCHMARK(BM_ThreadCounterStats)->Threads(3);

ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadCounterStats/threads:3 %console_report "
            "work=%hrfloat$"},
           {"^BM_ThreadCounterStats/threads:3_thread_mean %console_report "
            "work=%hrfloat$"},
           {"^BM_ThreadCounterStats/threads:3_thread_median %console_report "
            "work=%hrfloat$"},
           {"^BM_ThreadCounterStats/threads:3_thread_stddev %console_report "
            "work=%hrfloat$"},
           {"^BM_ThreadCounterStats/threads:3_thread_cv "
            "%console_percentage_report work=%hrfloat%$"}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_ThreadCounterStats/threads:3\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadCounterStats/threads:3\",$", MR_Next},
           {"\"run_type\": \"iteration\",$", MR_Next},
           {"\"repetitions\": 1,$", MR_Next},
           {"\"repetition_index\": 0,$", MR_Next},
           {"\"threads\": 3,$", MR_Next},
           {"\"iterations\": %int,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\",$", MR_Next},
           {"\"work\": %float$", MR_Next},
           {"}", MR_Next},
           {"\"name\": \"BM_ThreadCounterStats/threads:3_thread_mean\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadCounterStats/threads:3\",$", MR_Next},
           {"\"run_type\": \"aggregate\",$", MR_Next},
           {"\"repetitions\": 1,$", MR_Next},
           {"\"threads\": 3,$", MR_Next},
           {"\"aggregate_name\": \"thread_mean\",$", MR_Next},
           {"\"aggregate_unit\": \"time\",$", MR_Next},
           {"\"iterations\": 3,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\",$", MR_Next},
           {"\"work\": 2\\.0*e\\+00$", MR_Next},
           {"}", MR_Next},
           {"\"name\": \"BM_ThreadCounterStats/threads:3_thread_median\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadCounterStats/threads:3\",$", MR_Next},
           {"\"run_type\": \"aggregate\",$", MR_Next},
           {"\"repetitions\": 1,$", MR_Next},
           {"\"threads\": 3,$", MR_Next},
           {"\"aggregate_name\": \"thread_median\",$", MR_Next},
           {"\"aggregate_unit\": \"time\",$", MR_Next},
           {"\"iterations\": 3,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\",$", MR_Next},
           {"\"work\": 2\\.0*e\\+00$", MR_Next},
           {"}", MR_Next},
           {"\"name\": \"BM_ThreadCounterStats/threads:3_thread_stddev\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadCounterStats/threads:3\",$", MR_Next},
           {"\"run_type\": \"aggregate\",$", MR_Next},
           {"\"repetitions\": 1,$", MR_Next},
           {"\"threads\": 3,$", MR_Next},
           {"\"aggregate_name\": \"thread_stddev\",$", MR_Next},
           {"\"aggregate_unit\": \"time\",$", MR_Next},
           {"\"iterations\": 3,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\",$", MR_Next},
           {"\"work\": %float$", MR_Next},
           {"}", MR_Next},
           {"\"name\": \"BM_ThreadCounterStats/threads:3_thread_cv\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadCounterStats/threads:3\",$", MR_Next},
           {"\"run_type\": \"aggregate\",$", MR_Next},
           {"\"repetitions\": 1,$", MR_Next},
           {"\"threads\": 3,$", MR_Next},
           {"\"aggregate_name\": \"thread_cv\",$", MR_Next},
           {"\"aggregate_unit\": \"percentage\",$", MR_Next},
           {"\"iterations\": 3,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\",$", MR_Next},
           {"\"work\": %float$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_CSVOut,
          {{"^\"BM_ThreadCounterStats/threads:3\",%csv_report,%float$"},
           {"^\"BM_ThreadCounterStats/threads:3_thread_mean\",%csv_report,%float$"},
           {"^\"BM_ThreadCounterStats/threads:3_thread_median\",%csv_report,%float$"},
           {"^\"BM_ThreadCounterStats/threads:3_thread_stddev\",%csv_report,%float$"},
           {"^\"BM_ThreadCounterStats/threads:3_thread_cv\",%csv_cv_report,%float$"}});

void CheckThreadCounterStats(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "work", EQ, 6);
}
CHECK_BENCHMARK_RESULTS("^BM_ThreadCounterStats/threads:3$",
                        &CheckThreadCounterStats);

void CheckThreadCounterMean(Results const& e) {
  CHECK_FLOAT_COUNTER_VALUE(e, "work", EQ, 2.0, 0.001);
}
CHECK_BENCHMARK_RESULTS("^BM_ThreadCounterStats/threads:3_thread_mean$",
                        &CheckThreadCounterMean);

void CheckThreadCounterMedian(Results const& e) {
  CHECK_FLOAT_COUNTER_VALUE(e, "work", EQ, 2.0, 0.001);
}
CHECK_BENCHMARK_RESULTS("^BM_ThreadCounterStats/threads:3_thread_median$",
                        &CheckThreadCounterMedian);

void CheckThreadCounterStdDev(Results const& e) {
  CHECK_FLOAT_COUNTER_VALUE(e, "work", EQ, 1.0, 0.001);
}
CHECK_BENCHMARK_RESULTS("^BM_ThreadCounterStats/threads:3_thread_stddev$",
                        &CheckThreadCounterStdDev);

void CheckThreadCounterCV(Results const& e) {
  CHECK_FLOAT_COUNTER_VALUE(e, "work", EQ, 0.5, 0.001);
}
CHECK_BENCHMARK_RESULTS("^BM_ThreadCounterStats/threads:3_thread_cv$",
                        &CheckThreadCounterCV);
}  // end namespace

int main(int argc, char* argv[]) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  RunOutputTests(argc, argv);
}
