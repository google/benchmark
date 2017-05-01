#ifndef TEST_OUTPUT_TEST_H
#define TEST_OUTPUT_TEST_H

#undef NDEBUG
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <sstream>

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

struct Results;
typedef std::function< void(Results const&) > ResultsCheckFn;

// Add a function to check the (CSV) results of a benchmark. These
// functions will be called only after the output was successfully
// checked.
// bm_name_pattern: a name or a regex which will be matched agains
//                  all the benchmark names. Matching benchmarks
//                  will be the subject of a call to fn
size_t AddChecker(const char* bm_name_pattern, ResultsCheckFn fn);

// Class to hold the (CSV!) results of a benchmark.
// It is passed in calls to checker functions.
struct Results {
  std::string name; // the benchmark name
  std::map< std::string, std::string > values;

  Results(const std::string& n) : name(n) {}

  int NumThreads() const;

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
  // NOTE: for counters, use GetCounterAs instead.
  template <class T>
  T GetAs(const char* entry_name) const;

  // counters are written as doubles, so they have to be read first
  // as a double, and only then converted to the asked type.
  template <class T>
  T GetCounterAs(const char* entry_name) const {
    double dval = GetAs< double >(entry_name);
    T tval = static_cast< T >(dval);
    return tval;
  }

  // get cpu_time or real_time in seconds
  double GetTime(const char* which) const;
};

template <class T>
T Results::GetAs(const char* entry_name) const {
  auto *sv = Get(entry_name);
  CHECK(sv != nullptr && !sv->empty());
  std::stringstream ss;
  ss << *sv;
  T out;
  ss >> out;
  CHECK(!ss.fail());
  return out;
}

//----------------------------------
// Macros to help in result checking. Do not use them with arguments causing
// side-effects.

#define _CHECK_RESULT_VALUE(entry, getfn, var_type, var_name, relationship, value) \
    CONCAT(CHECK_, relationship)                                        \
    (entry.getfn< var_type >(var_name), (value)) << "\n"                \
    << __FILE__ << ":" << __LINE__ << ": " << (entry).name << ":\n"     \
    << __FILE__ << ":" << __LINE__ << ": "                              \
    << "expected (" << #var_type << ")" << (var_name)                   \
    << "=" << (entry).getfn< var_type >(var_name)                       \
    << " to be " #relationship " to " << (value) << "\n"

// check with tolerance. eps_factor is the tolerance window, which is
// interpreted relative to value (eg, 0.1 means 10% of value).
#define _CHECK_FLOAT_RESULT_VALUE(entry, getfn, var_type, var_name, relationship, value, eps_factor) \
    CONCAT(CHECK_FLOAT_, relationship)                                  \
    (entry.getfn< var_type >(var_name), (value), (eps_factor) * (value)) << "\n" \
    << __FILE__ << ":" << __LINE__ << ": " << (entry).name << ":\n"     \
    << __FILE__ << ":" << __LINE__ << ": "                              \
    << "expected (" << #var_type << ")" << (var_name)                   \
    << "=" << (entry).getfn< var_type >(var_name)                       \
    << " to be " #relationship " to " << (value) << "\n"                \
    << __FILE__ << ":" << __LINE__ << ": "                              \
    << "with tolerance of " << (eps_factor) * (value)                   \
    << " (" << (eps_factor)*100. << "%), "                              \
    << "but delta was " << ((entry).getfn< var_type >(var_name) - (value)) \
    << " (" << (((entry).getfn< var_type >(var_name) - (value))         \
               /                                                        \
               ((value) > 1.e-5 || value < -1.e-5 ? value : 1.e-5)*100.) \
    << "%)"

#define CHECK_RESULT_VALUE(entry, var_type, var_name, relationship, value) \
    _CHECK_RESULT_VALUE(entry, GetAs, var_type, var_name, relationship, value)

#define CHECK_COUNTER_VALUE(entry, var_type, var_name, relationship, value) \
    _CHECK_RESULT_VALUE(entry, GetCounterAs, var_type, var_name, relationship, value)

#define CHECK_FLOAT_RESULT_VALUE(entry, var_name, relationship, value, eps_factor) \
    _CHECK_FLOAT_RESULT_VALUE(entry, GetAs, double, var_name, relationship, value, eps_factor)

#define CHECK_FLOAT_COUNTER_VALUE(entry, var_name, relationship, value, eps_factor) \
    _CHECK_FLOAT_RESULT_VALUE(entry, GetCounterAs, double, var_name, relationship, value, eps_factor)

#define CHECK_BENCHMARK_RESULTS(bm_name, checker_function)              \
    size_t CONCAT(dummy, __LINE__) = AddChecker(bm_name, checker_function)

// ========================================================================= //
// --------------------------- Misc Utilities ------------------------------ //
// ========================================================================= //

namespace {

const char* const dec_re = "[0-9]*[.]?[0-9]+([eE][-+][0-9]+)?";

}  //  end namespace

#endif  // TEST_OUTPUT_TEST_H
