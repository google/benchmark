
#undef NDEBUG

#include "benchmark/benchmark_api.h"
#include "benchmark/counter.h"
#include "benchmark/registration.h"
#include "benchmark/state.h"
#include "benchmark/utils.h"
#include "output_test.h"

// clang-format off

ADD_CASES(TC_ConsoleOut,
          {{"^[-]+$", MR_Next},
           {"^Benchmark %s Time %s CPU %s Iterations UserCounters...$", MR_Next},
           {"^[-]+$", MR_Next}});
ADD_CASES(TC_CSVOut, {{"%csv_header,\"work\""}});

// clang-format on

namespace {
void BM_ThreadStats(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.counters["work"] = static_cast<double>(state.thread_index() + 1);
}
BENCHMARK(BM_ThreadStats)->Threads(3)->ReportThreadStatistics();

ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadStats/threads:3 %console_report work=%hrfloat$"}});
ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadStats/threads:3_thread_mean %console_report "
            "work=%hrfloat$"}});
ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadStats/threads:3_thread_median %console_report "
            "work=%hrfloat$"}});
ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadStats/threads:3_thread_stddev %console_report "
            "work=%hrfloat$"}});
ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadStats/threads:3_thread_cv %console_percentage_report "
            "work=%percentage%$"}});

ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_ThreadStats/threads:3\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadStats/threads:3\",$", MR_Next},
           {"\"run_type\": \"iteration\",$", MR_Next},
           {"\"repetitions\": 1,$", MR_Next},
           {"\"repetition_index\": 0,$", MR_Next},
           {"\"threads\": 3,$", MR_Next},
           {"\"iterations\": %int,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\",$", MR_Next},
           {"\"work\": %float$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_ThreadStats/threads:3_thread_mean\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadStats/threads:3\",$", MR_Next},
           {"\"run_type\": \"aggregate\",$", MR_Next},
           {"\"repetitions\": 1,$", MR_Next},
           {"\"threads\": 3,$", MR_Next},
           {"\"aggregate_name\": \"thread_mean\",$", MR_Next},
           {"\"aggregate_unit\": \"time\",$", MR_Next},
           {"\"iterations\": 3,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\",$", MR_Next},
           {"\"work\": %float$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_ThreadStats/threads:3_thread_median\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadStats/threads:3\",$", MR_Next},
           {"\"run_type\": \"aggregate\",$", MR_Next},
           {"\"repetitions\": 1,$", MR_Next},
           {"\"threads\": 3,$", MR_Next},
           {"\"aggregate_name\": \"thread_median\",$", MR_Next},
           {"\"aggregate_unit\": \"time\",$", MR_Next},
           {"\"iterations\": 3,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\",$", MR_Next},
           {"\"work\": %float$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_ThreadStats/threads:3_thread_stddev\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadStats/threads:3\",$", MR_Next},
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
           {"}", MR_Next}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_ThreadStats/threads:3_thread_cv\",$"},
           {"\"family_index\": 0,$", MR_Next},
           {"\"per_family_instance_index\": 0,$", MR_Next},
           {"\"run_name\": \"BM_ThreadStats/threads:3\",$", MR_Next},
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
          {{"^\"BM_ThreadStats/threads:3\",%csv_report,%float$"}});
ADD_CASES(TC_CSVOut,
          {{"^\"BM_ThreadStats/threads:3_thread_mean\",%csv_report,%float$"}});
ADD_CASES(TC_CSVOut, {{"^\"BM_ThreadStats/threads:3_thread_median\",%csv_report,"
                       "%float$"}});
ADD_CASES(TC_CSVOut, {{"^\"BM_ThreadStats/threads:3_thread_stddev\",%csv_report,"
                       "%float$"}});
ADD_CASES(TC_CSVOut, {{"^\"BM_ThreadStats/threads:3_thread_cv\",%csv_cv_report,"
                       "%float$"}});

void CheckThreadStatsSum(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "work", EQ, 6);
}
void CheckThreadStatsMean(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "work", EQ, 2);
}
void CheckThreadStatsMedian(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "work", EQ, 2);
}
void CheckThreadStatsStdDev(Results const& e) {
  CHECK_FLOAT_COUNTER_VALUE(e, "work", EQ, 1.0, 0.001);
}
void CheckThreadStatsCV(Results const& e) {
  CHECK_FLOAT_COUNTER_VALUE(e, "work", EQ, 0.5, 0.001);
}
CHECK_BENCHMARK_RESULTS("BM_ThreadStats/threads:3$", &CheckThreadStatsSum);
CHECK_BENCHMARK_RESULTS("BM_ThreadStats/threads:3_thread_mean$",
                        &CheckThreadStatsMean);
CHECK_BENCHMARK_RESULTS("BM_ThreadStats/threads:3_thread_median$",
                        &CheckThreadStatsMedian);
CHECK_BENCHMARK_RESULTS("BM_ThreadStats/threads:3_thread_stddev$",
                        &CheckThreadStatsStdDev);
CHECK_BENCHMARK_RESULTS("BM_ThreadStats/threads:3_thread_cv$",
                        &CheckThreadStatsCV);
}  // namespace

namespace {
void BM_ThreadStatsAvg(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.counters["work"] = benchmark::Counter{
      static_cast<double>(state.thread_index() + 1),
      benchmark::Counter::kAvgThreads};
}
BENCHMARK(BM_ThreadStatsAvg)->Threads(3)->ReportThreadStatistics();

ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadStatsAvg/threads:3 %console_report work=%hrfloat$"}});
ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadStatsAvg/threads:3_thread_mean %console_report "
            "work=%hrfloat$"}});

void CheckThreadStatsAvgSum(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "work", EQ, 2);
}
void CheckThreadStatsAvgMean(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "work", EQ, 2);
}
CHECK_BENCHMARK_RESULTS("BM_ThreadStatsAvg/threads:3$",
                        &CheckThreadStatsAvgSum);
CHECK_BENCHMARK_RESULTS("BM_ThreadStatsAvg/threads:3_thread_mean$",
                        &CheckThreadStatsAvgMean);
}  // namespace

namespace {
void BM_ThreadStatsOff(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.counters["work"] = static_cast<double>(state.thread_index() + 1);
}
BENCHMARK(BM_ThreadStatsOff)->Threads(3);

ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadStatsOff/threads:3 %console_report work=%hrfloat$"}});
ADD_CASES(TC_ConsoleOut, {{".*BM_ThreadStatsOff/threads:3_thread_", MR_Not}});

void CheckThreadStatsOff(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "work", EQ, 6);
}
CHECK_BENCHMARK_RESULTS("BM_ThreadStatsOff/threads:3$", &CheckThreadStatsOff);
}  // namespace

namespace {
void BM_ThreadStatsSingle(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.counters["work"] = 7;
}
BENCHMARK(BM_ThreadStatsSingle)->Threads(1)->ReportThreadStatistics();

ADD_CASES(TC_ConsoleOut,
          {{"^BM_ThreadStatsSingle/threads:1 %console_report work=%hrfloat$"}});
ADD_CASES(TC_ConsoleOut,
          {{".*BM_ThreadStatsSingle/threads:1_thread_", MR_Not}});

void CheckThreadStatsSingle(Results const& e) {
  CHECK_COUNTER_VALUE(e, int, "work", EQ, 7);
}
CHECK_BENCHMARK_RESULTS("BM_ThreadStatsSingle/threads:1$",
                        &CheckThreadStatsSingle);
}  // namespace

int main(int argc, char* argv[]) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  RunOutputTests(argc, argv);
}
