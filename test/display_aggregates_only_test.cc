
#undef NDEBUG
#include <cstdio>
#include <fstream>
#include <streambuf>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"
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
  std::vector<char*> new_argv(argv, argv + argc);
  assert(static_cast<decltype(new_argv)::size_type>(argc) == new_argv.size());

  std::string tmp_file_name = std::tmpnam(nullptr);
  std::cout << "Will be using this as the tmp file: " << tmp_file_name << '\n';

  std::string tmp = "--benchmark_out=";
  tmp += tmp_file_name;
  new_argv.emplace_back(const_cast<char*>(tmp.c_str()));

  argc = int(new_argv.size());

  benchmark::Initialize(&argc, new_argv.data());
  benchmark::RunSpecifiedBenchmarks();

  // Read the output back from the file, and delete the file.
  std::ifstream tmp_stream(tmp_file_name);
  std::string JsonOutput((std::istreambuf_iterator<char>(tmp_stream)),
                         std::istreambuf_iterator<char>());
  std::remove(tmp_file_name.c_str());

  if (SubstrCnt(JsonOutput, "BM_SummaryRepeat/repeats:3") != 6 ||
      SubstrCnt(JsonOutput, "\"BM_SummaryRepeat/repeats:3\"") != 3 ||
      SubstrCnt(JsonOutput, "\"BM_SummaryRepeat/repeats:3_mean\"") != 1 ||
      SubstrCnt(JsonOutput, "\"BM_SummaryRepeat/repeats:3_median\"") != 1 ||
      SubstrCnt(JsonOutput, "\"BM_SummaryRepeat/repeats:3_stddev\"") != 1) {
    std::cout << "Precondition mismatch. Expected to only find 6 "
                 "occurrences of \"BM_SummaryRepeat/repeats:3\" substring:\n"
                 "\"BM_SummaryRepeat/repeats:3\", "
                 "\"BM_SummaryRepeat/repeats:3\", "
                 "\"BM_SummaryRepeat/repeats:3\", "
                 "\"BM_SummaryRepeat/repeats:3_mean\", "
                 "\"BM_SummaryRepeat/repeats:3_median\", "
                 "\"BM_SummaryRepeat/repeats:3_stddev\"\nThe entire output:\n";
    std::cout << JsonOutput;
    return 1;
  }

  return 0;
}
