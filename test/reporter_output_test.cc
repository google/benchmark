
#undef NDEBUG
#include "benchmark/benchmark.h"

#include "test_reporter.h"

#include <cstring>
#include <iostream>
#include <vector>
#include <utility>

// ========================================================================= //
// ---------------------- Testing Prologue Output -------------------------- //
// ========================================================================= //

ADD_CASES(&test::ConsoleOutputTests,
          {{test::join("^Benchmark", "Time", "CPU", "Iterations$"),
            test::MR_Next},
           {"^[-]+$", test::MR_Next}});
ADD_CASES(&test::CSVOutputTests, {{"name,iterations,real_time,cpu_time,time_"
                                   "unit,bytes_per_second,items_per_second,"
                                   "label,error_occurred,error_message"}});

// ========================================================================= //
// ------------------------ Testing Basic Output --------------------------- //
// ========================================================================= //

void BM_basic(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
}
BENCHMARK(BM_basic);

ADD_CASES(&test::ConsoleOutputTests,
          {{"^BM_basic[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"}});
ADD_CASES(&test::JSONOutputTests,
          {{"\"name\": \"BM_basic\",$"},
           {"\"iterations\": [0-9]+,$", test::MR_Next},
           {"\"real_time\": [0-9]{1,5},$", test::MR_Next},
           {"\"cpu_time\": [0-9]{1,5},$", test::MR_Next},
           {"\"time_unit\": \"ns\"$", test::MR_Next},
           {"}", test::MR_Next}});
ADD_CASES(&test::CSVOutputTests,
          {{"^\"BM_basic\",[0-9]+," + std::string(test::dec_re) + "," +
            std::string(test::dec_re) + ",ns,,,,,$"}});

// ========================================================================= //
// ------------------------ Testing Error Output --------------------------- //
// ========================================================================= //

void BM_error(benchmark::State& state) {
  state.SkipWithError("message");
  while (state.KeepRunning()) {
  }
}
BENCHMARK(BM_error);
ADD_CASES(&test::ConsoleOutputTests,
          {{"^BM_error[ ]+ERROR OCCURRED: 'message'$"}});
ADD_CASES(&test::JSONOutputTests,
          {{"\"name\": \"BM_error\",$"},
           {"\"error_occurred\": true,$", test::MR_Next},
           {"\"error_message\": \"message\",$", test::MR_Next}});

ADD_CASES(&test::CSVOutputTests, {{"^\"BM_error\",,,,,,,,true,\"message\"$"}});

// ========================================================================= //
// ----------------------- Testing Complexity Output ----------------------- //
// ========================================================================= //

void BM_Complexity_O1(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.SetComplexityN(state.range_x());
}
BENCHMARK(BM_Complexity_O1)->Range(1, 1 << 18)->Complexity(benchmark::o1);

std::string bigOStr = "[0-9]+\\.[0-9]+ \\([0-9]+\\)";

ADD_CASES(&test::ConsoleOutputTests,
          {{test::join("^BM_Complexity_O1_BigO", bigOStr, bigOStr) + "[ ]*$"},
           {test::join("^BM_Complexity_O1_RMS", "[0-9]+ %", "[0-9]+ %") +
            "[ ]*$"}});

// ========================================================================= //
// --------------------------- TEST CASES END ------------------------------ //
// ========================================================================= //

int main(int argc, char* argv[]) {
  // Add --color_print=false to argv since we don't want to match color codes.
  char new_arg[64];
  char* new_argv[64];
  std::copy(argv, argv + argc, new_argv);
  new_argv[argc++] = std::strcpy(new_arg, "--color_print=false");
  benchmark::Initialize(&argc, new_argv);

  auto test_cases = test::CreateTestCases();

  std::vector<benchmark::BenchmarkReporter*> reporters;
  for (const auto& tc : test_cases) {
    reporters.push_back(tc->reporter);
  }
  test::TestReporter test_rep(reporters);

  std::cout << "Running benchmarks...\n";
  benchmark::RunSpecifiedBenchmarks(&test_rep);

  for (auto* tc : test_cases) {
    std::string msg = std::string("\nTesting ") + tc->name + " Output\n";
    std::string banner(msg.size() - 1, '-');
    std::cout << banner << msg << banner << "\n";

    std::cerr << tc->err_stream->str();
    std::cout << tc->out_stream->str();

    for (const auto& ec : *tc->error_cases) ec.Check(tc->err_stream.get());
    for (const auto& oc : *tc->output_cases) oc.Check(tc->out_stream.get());

    std::cout << "\n";
  }
  return 0;
}
