
#include "benchmark/benchmark.h"
#include "output_test.h"

void BM_empty(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(state.iterations());
  }
}
BENCHMARK(BM_empty)
    ->RangeMultiplier(2)
    ->Ranges({{1, 8}, {1, 1}})
    ->MinTime(0.001)
    ->MinWarmUpTime(0.001);

// arguments array should not contain general settings like min_time.
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_empty/.*\",$"},
                       {"\"family_index\": 0,$", MR_Next},
                       {"\"per_family_instance_index\": 0,$", MR_Next},
                       {"\"run_name\": \"BM_empty/.*\",$", MR_Next},
                       {"\"function_name\": \"BM_empty\",$", MR_Next},
                       {"\"arguments\": \\[\"1\", \"1\"],$", MR_Next},
                       {"\"run_type\": \"iteration\",$", MR_Next},
                       {"\"repetitions\": 1,$", MR_Next},
                       {"\"repetition_index\": 0,$", MR_Next},
                       {"\"threads\": 1,$", MR_Next},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\"$", MR_Next},
                       {"}", MR_Next}});

ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_empty/.*\",$"},
                       {"\"family_index\": 0,$", MR_Next},
                       {"\"per_family_instance_index\": 1,$", MR_Next},
                       {"\"run_name\": \"BM_empty/.*\",$", MR_Next},
                       {"\"function_name\": \"BM_empty\",$", MR_Next},
                       {"\"arguments\": \\[\"2\", \"1\"],$", MR_Next},
                       {"\"run_type\": \"iteration\",$", MR_Next},
                       {"\"repetitions\": 1,$", MR_Next},
                       {"\"repetition_index\": 0,$", MR_Next},
                       {"\"threads\": 1,$", MR_Next},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\"$", MR_Next},
                       {"}", MR_Next}});

ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_empty/.*\",$"},
                       {"\"family_index\": 0,$", MR_Next},
                       {"\"per_family_instance_index\": 2,$", MR_Next},
                       {"\"run_name\": \"BM_empty/.*\",$", MR_Next},
                       {"\"function_name\": \"BM_empty\",$", MR_Next},
                       {"\"arguments\": \\[\"4\", \"1\"],$", MR_Next},
                       {"\"run_type\": \"iteration\",$", MR_Next},
                       {"\"repetitions\": 1,$", MR_Next},
                       {"\"repetition_index\": 0,$", MR_Next},
                       {"\"threads\": 1,$", MR_Next},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\"$", MR_Next},
                       {"}", MR_Next}});

ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_empty/.*\",$"},
                       {"\"family_index\": 0,$", MR_Next},
                       {"\"per_family_instance_index\": 3,$", MR_Next},
                       {"\"run_name\": \"BM_empty/.*\",$", MR_Next},
                       {"\"function_name\": \"BM_empty\",$", MR_Next},
                       {"\"arguments\": \\[\"8\", \"1\"],$", MR_Next},
                       {"\"run_type\": \"iteration\",$", MR_Next},
                       {"\"repetitions\": 1,$", MR_Next},
                       {"\"repetition_index\": 0,$", MR_Next},
                       {"\"threads\": 1,$", MR_Next},
                       {"\"iterations\": %int,$", MR_Next},
                       {"\"real_time\": %float,$", MR_Next},
                       {"\"cpu_time\": %float,$", MR_Next},
                       {"\"time_unit\": \"ns\"$", MR_Next},
                       {"}", MR_Next}});

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
