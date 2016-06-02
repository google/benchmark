
#undef NDEBUG
#include "benchmark/benchmark.h"

#include "test_reporter.h"

#include <cstring>
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>

namespace {

#define ADD_COMPLEXITY_CASES(...) \
  int CONCAT(dummy, __LINE__) = AddComplexityTest(__VA_ARGS__)

int AddComplexityTest(std::vector<test::TestCase>* console_out,
                      std::vector<test::TestCase>* json_out,
                      std::vector<test::TestCase>* csv_out,
                      std::string big_o_test_name, std::string rms_test_name,
                      std::string big_o) {
  std::string big_o_str = std::string(test::dec_re) + " " + big_o;
  test::AddCases(
      console_out,
      {{test::join("^" + big_o_test_name + "", big_o_str, big_o_str) + "[ ]*$"},
       {test::join("^" + rms_test_name + "", "[0-9]+ %", "[0-9]+ %") +
        "[ ]*$"}});
  test::AddCases(json_out,
                 {{"\"name\": \"" + big_o_test_name + "\",$"},
                  {"\"cpu_coefficient\": [0-9]+,$", test::MR_Next},
                  {"\"real_coefficient\": [0-9]{1,5},$", test::MR_Next},
                  {"\"big_o\": \"" + big_o + "\",$", test::MR_Next},
                  {"\"time_unit\": \"ns\"$", test::MR_Next},
                  {"}", test::MR_Next},
                  {"\"name\": \"" + rms_test_name + "\",$"},
                  {"\"rms\": [0-9]+%$", test::MR_Next},
                  {"}", test::MR_Next}});
  test::AddCases(csv_out, {{"^\"" + big_o_test_name + "\",," + test::dec_re +
                            "," + test::dec_re + "," + big_o + ",,,,,$"},
                           {"^\"" + rms_test_name + "\",," + test::dec_re +
                            "," + test::dec_re + ",,,,,,$"}});
  return 0;
}

}  // end namespace

// ========================================================================= //
// --------------------------- Testing BigO O(1) --------------------------- //
// ========================================================================= //

void BM_Complexity_O1(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.SetComplexityN(state.range_x());
}
BENCHMARK(BM_Complexity_O1)->Range(1, 1 << 18)->Complexity(benchmark::o1);
BENCHMARK(BM_Complexity_O1)->Range(1, 1 << 18)->Complexity([](int) {
  return 1.0;
});
BENCHMARK(BM_Complexity_O1)->Range(1, 1 << 18)->Complexity();

const char big_o_1_test_name[] = "BM_Complexity_O1_BigO";
const char rms_o_1_test_name[] = "BM_Complexity_O1_RMS";
const char enum_auto_big_o_1[] = "\\([0-9]+\\)";
const char lambda_big_o_1[] = "f\\(N\\)";

// Add enum tests
ADD_COMPLEXITY_CASES(&test::ConsoleOutputTests, &test::JSONOutputTests,
                     &test::CSVOutputTests, big_o_1_test_name,
                     rms_o_1_test_name, enum_auto_big_o_1);

// Add lambda tests
ADD_COMPLEXITY_CASES(&test::ConsoleOutputTests, &test::JSONOutputTests,
                     &test::CSVOutputTests, big_o_1_test_name,
                     rms_o_1_test_name, lambda_big_o_1);

// ========================================================================= //
// --------------------------- Testing BigO O(N) --------------------------- //
// ========================================================================= //

std::vector<int> ConstructRandomVector(int size) {
  std::vector<int> v;
  v.reserve(size);
  for (int i = 0; i < size; ++i) {
    v.push_back(rand() % size);
  }
  return v;
}

void BM_Complexity_O_N(benchmark::State& state) {
  auto v = ConstructRandomVector(state.range_x());
  const int item_not_in_vector =
      state.range_x() * 2;  // Test worst case scenario (item not in vector)
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(std::find(v.begin(), v.end(), item_not_in_vector));
  }
  state.SetComplexityN(state.range_x());
}
BENCHMARK(BM_Complexity_O_N)
    ->RangeMultiplier(2)
    ->Range(1 << 10, 1 << 16)
    ->Complexity(benchmark::oN);
BENCHMARK(BM_Complexity_O_N)
    ->RangeMultiplier(2)
    ->Range(1 << 10, 1 << 16)
    ->Complexity([](int n) -> double { return n; });
BENCHMARK(BM_Complexity_O_N)
    ->RangeMultiplier(2)
    ->Range(1 << 10, 1 << 16)
    ->Complexity();

const char big_o_n_test_name[] = "BM_Complexity_O_N_BigO";
const char rms_o_n_test_name[] = "BM_Complexity_O_N_RMS";
const char enum_auto_big_o_n[] = "N";
const char lambda_big_o_n[] = "f\\(N\\)";

// Add enum tests
ADD_COMPLEXITY_CASES(&test::ConsoleOutputTests, &test::JSONOutputTests,
                     &test::CSVOutputTests, big_o_n_test_name,
                     rms_o_n_test_name, enum_auto_big_o_n);

// Add lambda tests
ADD_COMPLEXITY_CASES(&test::ConsoleOutputTests, &test::JSONOutputTests,
                     &test::CSVOutputTests, big_o_n_test_name,
                     rms_o_n_test_name, lambda_big_o_n);

// ========================================================================= //
// ------------------------- Testing BigO O(N*lgN) ------------------------- //
// ========================================================================= //

static void BM_Complexity_O_N_log_N(benchmark::State& state) {
  auto v = ConstructRandomVector(state.range_x());
  while (state.KeepRunning()) {
    std::sort(v.begin(), v.end());
  }
  state.SetComplexityN(state.range_x());
}
BENCHMARK(BM_Complexity_O_N_log_N)
    ->RangeMultiplier(2)
    ->Range(1 << 10, 1 << 16)
    ->Complexity(benchmark::oNLogN);
BENCHMARK(BM_Complexity_O_N_log_N)
    ->RangeMultiplier(2)
    ->Range(1 << 10, 1 << 16)
    ->Complexity([](int n) { return n * log2(n); });
BENCHMARK(BM_Complexity_O_N_log_N)
    ->RangeMultiplier(2)
    ->Range(1 << 10, 1 << 16)
    ->Complexity();

const char big_o_n_lg_n_test_name[] = "BM_Complexity_O_N_log_N_BigO";
const char rms_o_n_lg_n_test_name[] = "BM_Complexity_O_N_log_N_RMS";
const char enum_auto_big_o_n_lg_n[] = "NlgN";
const char lambda_big_o_n_lg_n[] = "f\\(N\\)";

// Add enum tests
ADD_COMPLEXITY_CASES(&test::ConsoleOutputTests, &test::JSONOutputTests,
                     &test::CSVOutputTests, big_o_n_lg_n_test_name,
                     rms_o_n_lg_n_test_name, enum_auto_big_o_n_lg_n);

// Add lambda tests
ADD_COMPLEXITY_CASES(&test::ConsoleOutputTests, &test::JSONOutputTests,
                     &test::CSVOutputTests, big_o_n_lg_n_test_name,
                     rms_o_n_lg_n_test_name, lambda_big_o_n_lg_n);

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
  for (const auto tc : test_cases) {
    reporters.push_back(tc->reporter);
  }
  test::TestReporter test_rep(reporters);

  benchmark::RunSpecifiedBenchmarks(&test_rep);

  for (auto* tc : test_cases) {
    std::string msg = std::string("\nTesting ") + tc->name + " Output\n";
    std::string banner(msg.size() - 1, '-');
    std::cout << banner << msg << banner << "\n";

    std::cerr << tc->err_stream->str();
    std::cout << tc->out_stream->str();

    for (const auto& oc : *tc->output_cases) oc.Check(tc->out_stream.get());

    std::cout << "\n";
  }
  return 0;
}
