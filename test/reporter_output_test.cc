
#undef NDEBUG
#include "benchmark/benchmark.h"
#include "../src/check.h" // NOTE: check.h is for internal use only!
#include "../src/re.h" // NOTE: re.h is for internal use only
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>
#include <utility>

namespace {

// ========================================================================= //
// -------------------------- Testing Case --------------------------------- //
// ========================================================================= //

enum MatchRules {
  MR_Default, // Skip non-matching lines until a match is found.
  MR_Next,    // Match must occur on the next line.
  MR_Not      // No line between the current position and the next match matches
              // the regex
};

struct TestCase {
  std::string regex_str;
  int match_rule;
  std::shared_ptr<benchmark::Regex> regex;

  TestCase(std::string re, int rule = MR_Default)
      : regex_str(re), match_rule(rule), regex(std::make_shared<benchmark::Regex>()) {
    std::string err_str;
    regex->Init(regex_str, &err_str);
    CHECK(err_str.empty()) << "Could not construct regex \"" << regex_str << "\""
                           << " got Error: " << err_str;
  }

  void Check(std::stringstream& remaining_output,
             std::vector<TestCase>& not_checks) const {
    std::string line;
    while (remaining_output.eof() == false) {
        CHECK(remaining_output.good());
        std::getline(remaining_output, line);
        for (auto& NC : not_checks) {
            CHECK(!NC.regex->Match(line)) << "Unexpected match for line \""
                                          << line << "\" for MR_Not regex \""
                                          << NC.regex_str << "\"";
        }
        if (regex->Match(line)) return;
        CHECK(match_rule != MR_Next) << "Expected line \"" << line
                                     << "\" to match regex \"" << regex_str << "\"";
    }

    CHECK(remaining_output.eof() == false)
        << "End of output reached before match for regex \"" << regex_str
        << "\" was found";
  }

  static void CheckCases(std::vector<TestCase> const& checks,
                         std::stringstream& output) {
      std::vector<TestCase> not_checks;
      for (size_t i=0; i < checks.size(); ++i) {
          const auto& TC = checks[i];
          if (TC.match_rule == MR_Not) {
              not_checks.push_back(TC);
              continue;
          }
          TC.Check(output, not_checks);
          not_checks.clear();
      }
  }
};

std::vector<TestCase> ConsoleOutputTests;
std::vector<TestCase> JSONOutputTests;
std::vector<TestCase> CSVOutputTests;

std::vector<TestCase> ConsoleErrorTests;
std::vector<TestCase> JSONErrorTests;
std::vector<TestCase> CSVErrorTests;

// ========================================================================= //
// -------------------------- Test Helpers --------------------------------- //
// ========================================================================= //

class TestReporter : public benchmark::BenchmarkReporter {
public:
  TestReporter(std::vector<benchmark::BenchmarkReporter*> reps)
      : reporters_(reps)  {}

  virtual bool ReportContext(const Context& context) {
    bool last_ret = false;
    bool first = true;
    for (auto rep : reporters_) {
      bool new_ret = rep->ReportContext(context);
      CHECK(first || new_ret == last_ret)
          << "Reports return different values for ReportContext";
      first = false;
      last_ret = new_ret;
    }
    return last_ret;
  }

  virtual void ReportRuns(const std::vector<Run>& report) {
    for (auto rep : reporters_)
      rep->ReportRuns(report);
  }

  virtual void Finalize() {
      for (auto rep : reporters_)
        rep->Finalize();
  }

private:
  std::vector<benchmark::BenchmarkReporter*> reporters_;
};


#define CONCAT2(x, y) x##y
#define CONCAT(x, y) CONCAT2(x, y)

#define ADD_CASES(...) \
    int CONCAT(dummy, __LINE__) = AddCases(__VA_ARGS__)

int AddCases(std::vector<TestCase>* out, std::initializer_list<TestCase> const& v) {
  for (auto const& TC : v)
    out->push_back(TC);
  return 0;
}

template <class First>
std::string join(First f) { return f; }

template <class First, class ...Args>
std::string join(First f, Args&&... args) {
    return std::string(std::move(f)) + "[ ]+" + join(std::forward<Args>(args)...);
}

std::string dec_re = "[0-9]*[.]?[0-9]+([eE][-+][0-9]+)?";

}  // end namespace

// ========================================================================= //
// ---------------------- Testing Prologue Output -------------------------- //
// ========================================================================= //

ADD_CASES(&ConsoleOutputTests, {
    {join("^Benchmark", "Time", "CPU", "Iterations$"), MR_Next},
    {"^[-]+$", MR_Next}
});
ADD_CASES(&CSVOutputTests, {
  {"name,iterations,real_time,cpu_time,time_unit,bytes_per_second,items_per_second,"
    "label,error_occurred,error_message"}
});

// ========================================================================= //
// ------------------------ Testing Basic Output --------------------------- //
// ========================================================================= //

void BM_basic(benchmark::State& state) {
  while (state.KeepRunning()) {}
}
BENCHMARK(BM_basic);

ADD_CASES(&ConsoleOutputTests, {
    {"^BM_basic[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"}
});
ADD_CASES(&JSONOutputTests, {
    {"\"name\": \"BM_basic\",$"},
    {"\"iterations\": [0-9]+,$", MR_Next},
    {"\"real_time\": [0-9]{1,5},$", MR_Next},
    {"\"cpu_time\": [0-9]{1,5},$", MR_Next},
    {"\"time_unit\": \"ns\"$", MR_Next},
    {"}", MR_Next}
});
ADD_CASES(&CSVOutputTests, {
    {"^\"BM_basic\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"}
});

// ========================================================================= //
// ------------------------ Testing Error Output --------------------------- //
// ========================================================================= //

void BM_error(benchmark::State& state) {
    state.SkipWithError("message");
    while(state.KeepRunning()) {}
}
BENCHMARK(BM_error);
ADD_CASES(&ConsoleOutputTests, {
    {"^BM_error[ ]+ERROR OCCURRED: 'message'$"}
});
ADD_CASES(&JSONOutputTests, {
    {"\"name\": \"BM_error\",$"},
    {"\"error_occurred\": true,$", MR_Next},
    {"\"error_message\": \"message\",$", MR_Next}
});

ADD_CASES(&CSVOutputTests, {
    {"^\"BM_error\",,,,,,,,true,\"message\"$"}
});


// ========================================================================= //
// ----------------------- Testing Complexity Output ----------------------- //
// ========================================================================= //

void BM_Complexity_O1(benchmark::State& state) {
  while (state.KeepRunning()) {
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_Complexity_O1)->Range(1, 1<<18)->Complexity(benchmark::o1);

std::string bigOStr = "[0-9]+\\.[0-9]+ \\([0-9]+\\)";

ADD_CASES(&ConsoleOutputTests, {
   {join("^BM_Complexity_O1_BigO", bigOStr, bigOStr) + "[ ]*$"},
   {join("^BM_Complexity_O1_RMS", "[0-9]+ %", "[0-9]+ %") + "[ ]*$"}
});


// ========================================================================= //
// ----------------------- Testing Aggregate Output ------------------------ //
// ========================================================================= //

// Test that non-aggregate data is printed by default
void BM_Repeat(benchmark::State& state) { while (state.KeepRunning()) {} }
BENCHMARK(BM_Repeat)->Repetitions(3);
ADD_CASES(&ConsoleOutputTests, {
    {"^BM_Repeat/repeats:3[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"},
    {"^BM_Repeat/repeats:3[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"},
    {"^BM_Repeat/repeats:3[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"},
    {"^BM_Repeat/repeats:3_mean[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"},
    {"^BM_Repeat/repeats:3_stddev[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"}
});
ADD_CASES(&JSONOutputTests, {
    {"\"name\": \"BM_Repeat/repeats:3\",$"},
    {"\"name\": \"BM_Repeat/repeats:3\",$"},
    {"\"name\": \"BM_Repeat/repeats:3\",$"},
    {"\"name\": \"BM_Repeat/repeats:3_mean\",$"},
    {"\"name\": \"BM_Repeat/repeats:3_stddev\",$"}
});
ADD_CASES(&CSVOutputTests, {
    {"^\"BM_Repeat/repeats:3\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"},
    {"^\"BM_Repeat/repeats:3\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"},
    {"^\"BM_Repeat/repeats:3\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"},
    {"^\"BM_Repeat/repeats:3_mean\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"},
    {"^\"BM_Repeat/repeats:3_stddev\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"}
});

// Test that a non-repeated test still prints non-aggregate results even when
// only-aggregate reports have been requested
void BM_RepeatOnce(benchmark::State& state) { while (state.KeepRunning()) {} }
BENCHMARK(BM_RepeatOnce)->Repetitions(1)->ReportAggregatesOnly();
ADD_CASES(&ConsoleOutputTests, {
    {"^BM_RepeatOnce/repeats:1[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"}
});
ADD_CASES(&JSONOutputTests, {
    {"\"name\": \"BM_RepeatOnce/repeats:1\",$"}
});
ADD_CASES(&CSVOutputTests, {
    {"^\"BM_RepeatOnce/repeats:1\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"}
});

// Test that non-aggregate data is not reported
void BM_SummaryRepeat(benchmark::State& state) { while (state.KeepRunning()) {} }
BENCHMARK(BM_SummaryRepeat)->Repetitions(3)->ReportAggregatesOnly();
ADD_CASES(&ConsoleOutputTests, {
    {".*BM_SummaryRepeat/repeats:3 ", MR_Not},
    {"^BM_SummaryRepeat/repeats:3_mean[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"},
    {"^BM_SummaryRepeat/repeats:3_stddev[ ]+[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+$"}
});
ADD_CASES(&JSONOutputTests, {
    {".*BM_SummaryRepeat/repeats:3 ", MR_Not},
    {"\"name\": \"BM_SummaryRepeat/repeats:3_mean\",$"},
    {"\"name\": \"BM_SummaryRepeat/repeats:3_stddev\",$"}
});
ADD_CASES(&CSVOutputTests, {
    {".*BM_SummaryRepeat/repeats:3 ", MR_Not},
    {"^\"BM_SummaryRepeat/repeats:3_mean\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"},
    {"^\"BM_SummaryRepeat/repeats:3_stddev\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"}
});


// ========================================================================= //
// ------------------ Testing Custom Counter Output ------------------------ //
// ========================================================================= //

// Test that non-aggregate data is printed by default
void BM_CustomCounters(benchmark::State& state) {
    while (state.KeepRunning()) {}
    switch (state.range(0)) {
    case 0:
        state.counters["foo"] = 0;
        break;
    case 1:
        state.counters["bar"] = {42, benchmark::CT_Rate};
        break;
    case 2:
        state.counters["baz"] = {4, benchmark::CT_ThreadAverage};
        break;
    case 3:
        state.counters["boo"] = {8, benchmark::CT_ThreadAverageRate};
        break;
    case 4:
        state.counters["foo"] = 0;
        state.counters["bar"] = {42, benchmark::CT_Rate};
        break;
    default:
        assert(false && "Test case not supported");
    }
    //
}
BENCHMARK(BM_CustomCounters)->Arg(0)->Arg(1);
BENCHMARK(BM_CustomCounters)->Threads(4)->Arg(2)->Arg(3);
BENCHMARK(BM_CustomCounters)->Arg(4);
std::string ConsoleRE = "[0-9]{1,5} ns[ ]+[0-9]{1,5} ns[ ]+[0-9]+";
ADD_CASES(&ConsoleOutputTests, {
    {join("^BM_CustomCounters/0", ConsoleRE, "foo=0$")},
    {join("^BM_CustomCounters/1", ConsoleRE, "bar=" + dec_re + "/ns$")},
    {join("^BM_CustomCounters/2/threads:4", ConsoleRE, "baz=" + dec_re + "/thread$")},
    {join("^BM_CustomCounters/3/threads:4", ConsoleRE, "boo=" + dec_re + "/ns/thread$")},
    {join("^BM_CustomCounters/4", ConsoleRE, "((foo=0 bar=" + dec_re + "/ns)|(bar=" + dec_re + "/ns foo=0))$")}
});
static int MakeJSONCase(std::string name, std::string cname,
                        std::string value, std::string type)
{
    AddCases(&JSONOutputTests, {
        {"\"name\": \"" + name + "\",$"},
        {"\"counters\": \\["},
        {"\\{", MR_Next},
        {"\"name\": \"" + cname + "\",$", MR_Next},
        {"\"value\": " + value + ",$", MR_Next},
        {"\"type\": \"" + type + "\"$", MR_Next},
        {"}$", MR_Next},
        {"]$", MR_Next}
    });
    return 0;
}
int json_test_anchor = (
      MakeJSONCase("BM_CustomCounters/0", "foo", "0", "unit")
    , MakeJSONCase("BM_CustomCounters/1", "bar", dec_re, "unit/ns")
    , MakeJSONCase("BM_CustomCounters/2/threads:4", "baz", dec_re, "unit/thread")
    , MakeJSONCase("BM_CustomCounters/3/threads:4", "boo", dec_re, "unit/ns/thread")
);

/*
ADD_CASES(&CSVOutputTests, {
    {"^\"BM_Repeat/repeats:3\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"},
    {"^\"BM_Repeat/repeats:3\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"},
    {"^\"BM_Repeat/repeats:3\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"},
    {"^\"BM_Repeat/repeats:3_mean\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"},
    {"^\"BM_Repeat/repeats:3_stddev\",[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,$"}
});
*/

// ========================================================================= //
// --------------------------- TEST CASES END ------------------------------ //
// ========================================================================= //


int main(int argc, char* argv[]) {
  benchmark::Initialize(&argc, argv);
  benchmark::ConsoleReporter CR(benchmark::ConsoleReporter::OO_None);
  benchmark::JSONReporter JR;
  benchmark::CSVReporter CSVR;
  struct ReporterTest {
    const char* name;
    std::vector<TestCase>& output_cases;
    std::vector<TestCase>& error_cases;
    benchmark::BenchmarkReporter& reporter;
    std::stringstream out_stream;
    std::stringstream err_stream;

    ReporterTest(const char* n,
                 std::vector<TestCase>& out_tc,
                 std::vector<TestCase>& err_tc,
                 benchmark::BenchmarkReporter& br)
        : name(n), output_cases(out_tc), error_cases(err_tc), reporter(br) {
        reporter.SetOutputStream(&out_stream);
        reporter.SetErrorStream(&err_stream);
    }
  }
  TestCases[] = {
      {"ConsoleReporter", ConsoleOutputTests, ConsoleErrorTests, CR},
      {"JSONReporter", JSONOutputTests, JSONErrorTests, JR},
      {"CSVReporter", CSVOutputTests, CSVErrorTests, CSVR}
  };

  // Create the test reporter and run the benchmarks.
  std::cout << "Running benchmarks...\n";
  TestReporter test_rep({&CR, &JR, &CSVR});
  benchmark::RunSpecifiedBenchmarks(&test_rep);

  for (auto& rep_test : TestCases) {
      std::string msg = std::string("\nTesting ") + rep_test.name + " Output\n";
      std::string banner(msg.size() - 1, '-');
      std::cout << banner << msg << banner << "\n";

      std::cerr << rep_test.err_stream.str();
      std::cout << rep_test.out_stream.str();

      TestCase::CheckCases(rep_test.error_cases, rep_test.err_stream);
      TestCase::CheckCases(rep_test.output_cases, rep_test.out_stream);

      std::cout << "\n";
  }
  return 0;
}
