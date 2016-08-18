#ifndef TEST_OUTPUT_TEST_H
#define TEST_OUTPUT_TEST_H

#undef NDEBUG
#include "benchmark/benchmark.h"
#include "../src/re.h"
#include <vector>
#include <string>
#include <initializer_list>
#include <memory>
#include <utility>

#define CONCAT2(x, y) x##y
#define CONCAT(x, y) CONCAT2(x, y)

#define ADD_CASES(...) \
    int CONCAT(dummy, __LINE__) = ::AddCases(__VA_ARGS__)

#define SET_SUBSTITUTIONS(...) \
    int CONCAT(dummy, __LINE__) = ::SetSubstitutions(__VA_ARGS__)

enum MatchRules {
  MR_Default, // Skip non-matching lines until a match is found.
  MR_Next,    // Match must occur on the next line.
  MR_Not      // No line between the current position and the next match matches
              // the regex
};

struct TestCase {
  std::string regex_str;
  int match_rule;

  TestCase(std::string re, int rule = MR_Default);

  void Check(std::stringstream& remaining_output,
             std::vector<TestCase> const& not_cases) const;
private:
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
  TC_NumID
};

// Add a list of test cases to be run against the output specified by
// 'ID'
int AddCases(TestCaseID ID, std::initializer_list<TestCase> il);

// Add or set a list of substitutions to be performed on constructed regexes
int SetSubstitutions(
    std::initializer_list<std::pair<std::string, std::string>> il);

// Run all output tests.
void RunOutputTests(int argc, char* argv[]);

// ========================================================================= //
// --------------------------- Misc Utilities ------------------------------ //
// ========================================================================= //

namespace {

std::string dec_re = "[0-9]*[.]?[0-9]+([eE][-+][0-9]+)?";

int default_sub_anchor = SetSubstitutions({
        {"%float", dec_re},
        {"%int", "[ ]*[0-9]+"},
        {" %s ", "[ ]+"},
        {"%time", "[ ]*[0-9]{1,5} ns"},
        {"%console_report", "[ ]*[0-9]{1,5} ns [ ]*[0-9]{1,5} ns [ ]*[0-9]+"},
        {"%csv_report", "[0-9]+," + dec_re + "," + dec_re + ",ns,,,,,"}
});

} //  end namespace

#endif // TEST_OUTPUT_TEST_H