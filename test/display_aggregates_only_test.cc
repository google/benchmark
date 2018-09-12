
#undef NDEBUG
#include <cstdio>
#include <string>

#include "benchmark/benchmark.h"
#include "file_reporter_test_helper.h"
#include "output_test.h"

// Ok this test is super ugly. We want to check what happens with the file
// reporter in the presence of DisplayAggregatesOnly().
// We do not care about console output, the normal tests check that already.

void BM_SummaryRepeat(benchmark::State& state) {
  for (auto _ : state) {
  }
}
BENCHMARK(BM_SummaryRepeat)->Repetitions(3)->DisplayAggregatesOnly();

int main(int argc, char* argv[]) {
  TestBenchmarkFileReporter helper(argc, argv);
  const std::string& output = helper.getOutput();

  if (SubstrCnt(output, "BM_SummaryRepeat/repeats:3") != 6 ||
      SubstrCnt(output, "\"BM_SummaryRepeat/repeats:3\"") != 3 ||
      SubstrCnt(output, "\"BM_SummaryRepeat/repeats:3_mean\"") != 1 ||
      SubstrCnt(output, "\"BM_SummaryRepeat/repeats:3_median\"") != 1 ||
      SubstrCnt(output, "\"BM_SummaryRepeat/repeats:3_stddev\"") != 1) {
    std::cout << "Precondition mismatch. Expected to only find 6 "
                 "occurrences of \"BM_SummaryRepeat/repeats:3\" substring:\n"
                 "\"BM_SummaryRepeat/repeats:3\", "
                 "\"BM_SummaryRepeat/repeats:3\", "
                 "\"BM_SummaryRepeat/repeats:3\", "
                 "\"BM_SummaryRepeat/repeats:3_mean\", "
                 "\"BM_SummaryRepeat/repeats:3_median\", "
                 "\"BM_SummaryRepeat/repeats:3_stddev\"\nThe entire output:\n";
    std::cout << output;
    return 1;
  }

  return 0;
}
