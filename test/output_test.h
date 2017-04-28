#ifndef TEST_OUTPUT_TEST_H
#define TEST_OUTPUT_TEST_H

#undef NDEBUG
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../src/re.h"
#include "benchmark/benchmark.h"

#define CONCAT2(x, y) x##y
#define CONCAT(x, y) CONCAT2(x, y)

#define ADD_CASES(...) int CONCAT(dummy, __LINE__) = ::AddCases(__VA_ARGS__)

#define SET_SUBSTITUTIONS(...) \
  int CONCAT(dummy, __LINE__) = ::SetSubstitutions(__VA_ARGS__)

enum MatchRules {
  MR_Default,  // Skip non-matching lines until a match is found.
  MR_Next,     // Match must occur on the next line.
  MR_Not  // No line between the current position and the next match matches
          // the regex
};

struct TestCase {
  TestCase(std::string re, int rule = MR_Default);

  std::string regex_str;
  int match_rule;
  std::string substituted_regex;
  std::shared_ptr<benchmark::Regex> regex;
};

enum TestCaseID {
  TC_ConsoleOut,
  TC_ConsoleErr,
  TC_JSONOut,
  TC_JSONErr,
  TC_CSVOut,
  TC_CSVErr,

  TC_NumID  // PRIVATE
};

// Add a list of test cases to be run against the output specified by
// 'ID'
int AddCases(TestCaseID ID, std::initializer_list<TestCase> il);

// Add or set a list of substitutions to be performed on constructed regex's
// See 'output_test_helper.cc' for a list of default substitutions.
int SetSubstitutions(
    std::initializer_list<std::pair<std::string, std::string>> il);

// Run all output tests.
void RunOutputTests(int argc, char* argv[]);

// ========================================================================= //
// ------------------------- Results checking ------------------------------ //
// ========================================================================= //

struct ResultsCheckerEntry;
typedef std::function< void(ResultsCheckerEntry const&) > ResultsCheckFn;

// Class to test the results of a benchmark.
// It inspects the results by looking at the CSV output of a subscribed
// benchmark.
struct ResultsCheckerEntry {
  std::string name;
  std::map< std::string, std::string > values;
  ResultsCheckFn check_fn;

  int NumThreads() const {
    auto pos = name.find_last_of("/threads:");
    if(pos == name.npos) return 1;
    auto end = name.find_last_of('/', pos + 9);
    std::stringstream ss;
    ss << name.substr(pos + 9, end);
    int num = 1;
    ss >> num;
    CHECK(!ss.fail());
    return num;
  }

  // get the real_time duration of the benchmark in seconds
  double DurationRealTime() const {
    return GetAs< double >("iterations") * GetTime("real_time");
  }
  // get the cpu_time duration of the benchmark in seconds
  double DurationCPUTime() const {
    return GetAs< double >("iterations") * GetTime("cpu_time");
  }

  // get the string for a result by name, or nullptr if the name
  // is not found
  const std::string* Get(const char* entry_name) const {
    auto it = values.find(entry_name);
    if(it == values.end()) return nullptr;
    return &it->second;
  }

  // get a result by name, parsed as a specific type.
  // For counters, use GetCounterAs instead.
  template< class T > T GetAs(const char* entry_name) const {
    auto *sv = Get(entry_name);
    CHECK(sv != nullptr && !sv->empty());
    std::stringstream ss;
    ss << *sv;
    T out;
    ss >> out;
    CHECK(!ss.fail());
    return out;
  }

  // counters are written as doubles, so they have to be read first
  // as a double, and only then converted to the asked type.
  template< class T > T GetCounterAs(const char* entry_name) const {
    double dval = GetAs< double >(entry_name);
    T tval = static_cast< T >(dval);
    return tval;
  }

  // get cpu_time or real_time in seconds
  double GetTime(const char* which) const {
    double val = GetAs< double >(which);
    auto unit = Get("time_unit");
    CHECK(unit);
    if(*unit == "ns") {
      return val * 1.e-9;
    } else if(*unit == "us") {
      return val * 1.e-6;
    } else if(*unit == "ms") {
      return val * 1.e-3;
    } else if(*unit == "s") {
      return val;
    } else {
      CHECK(1 == 0) << "unknown time unit: " << *unit;
      return 0;
    }
  }
};

#define _CHECK_RESULT_VALUE(entry, getfn, var_type, var_name, relationship, value) \
    CONCAT(CHECK_, relationship)(entry.getfn< var_type >(var_name), (value)) \
                                << "\n" << __FILE__ << ":" << __LINE__ << ": "  \
                                << entry.name << ": expected (" << #var_type << ")" \
                                << var_name << "=" << entry.GetAs< var_type >(var_name) \
                                << " to be " #relationship " to " << (value);

#define CHECK_RESULT_VALUE(entry, var_type, var_name, relationship, value) \
    _CHECK_RESULT_VALUE(entry, GetAs, var_type, var_name, relationship, value)

#define CHECK_COUNTER_VALUE(entry, var_type, var_name, relationship, value) \
    _CHECK_RESULT_VALUE(entry, GetCounterAs, var_type, var_name, relationship, value)

#define CHECK_BENCHMARK_RESULTS(bm_name, checker_function)              \
    size_t CONCAT(dummy, __LINE__) = AddChecker(bm_name, checker_function)

// Add a function to check the (CSV) results of a benchmark. These
// functions will be called only after the output was successfully
// checked.
size_t AddChecker(const char* bm_name, ResultsCheckFn fn);

// ========================================================================= //
// --------------------------- Misc Utilities ------------------------------ //
// ========================================================================= //

namespace {

const char* const dec_re = "[0-9]*[.]?[0-9]+([eE][-+][0-9]+)?";

}  //  end namespace

#endif  // TEST_OUTPUT_TEST_H
