#include "benchmark/benchmark_api.h"
#include "benchmark/registration.h"
#include "benchmark/state.h"
#include "output_test.h"

namespace {
void BM_Empty(benchmark::State& state) {
  for (auto _ : state) {
  }
  state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_Empty)
    ->Name("BM_Tagged")
    ->Arg(1)
    ->Tag("lib1")
    ->Tag("slow")
    ->Tag("lib1");
BENCHMARK(BM_Empty)->Name("BM_Untagged")->Arg(1);
BENCHMARK(BM_Empty)->Name("BM_Reps")->Arg(1)->Tag("lib1")->Repetitions(2);
BENCHMARK(BM_Empty)
    ->Name("BM_TaggedBigO")
    ->Range(1, 8)
    ->Tag("lib1")
    ->Complexity(benchmark::oN);
}  // end namespace

// Tags keep the order they were added, without duplicates.
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Tagged/1\",$"},
                       {"\"family_index\": 0,$", MR_Next},
                       {"\"per_family_instance_index\": 0,$", MR_Next},
                       {"\"run_name\": \"BM_Tagged/1\",$", MR_Next},
                       {"\"tags\": \\[\"lib1\", \"slow\"],$", MR_Next},
                       {"\"run_type\": \"iteration\",$", MR_Next}});

// Untagged benchmarks emit no "tags" key, so existing output is unchanged.
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Untagged/1\",$"},
                       {"\"tags\"", MR_Not},
                       {"\"run_type\": \"iteration\",$"}});

// Aggregates and complexity rows keep the family's tags.
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_Reps/1/repeats:2_mean\",$"},
                       {"\"family_index\": 2,$", MR_Next},
                       {"\"per_family_instance_index\": 0,$", MR_Next},
                       {"\"run_name\": \"BM_Reps/1/repeats:2\",$", MR_Next},
                       {"\"tags\": \\[\"lib1\"],$", MR_Next},
                       {"\"run_type\": \"aggregate\",$", MR_Next}});
ADD_CASES(TC_JSONOut, {{"\"name\": \"BM_TaggedBigO_BigO\",$"},
                       {"\"family_index\": 3,$", MR_Next},
                       {"\"per_family_instance_index\": 0,$", MR_Next},
                       {"\"run_name\": \"BM_TaggedBigO\",$", MR_Next},
                       {"\"tags\": \\[\"lib1\"],$", MR_Next},
                       {"\"run_type\": \"aggregate\",$", MR_Next}});

int main(int argc, char* argv[]) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  RunOutputTests(argc, argv);
}
