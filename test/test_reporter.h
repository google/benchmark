#ifndef TEST_TEST_REPORTER_H_
#define TEST_TEST_REPORTER_H_

#include "benchmark/benchmark.h"

#include "../src/check.h"  // NOTE: check.h is for internal use only!
#include "../src/re.h"     // NOTE: re.h is for internal use only

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace test {

// ========================================================================= //
// -------------------------- Testing Case --------------------------------- //
// ========================================================================= //

enum MatchRules {
  MR_Default,  // Skip non-matching lines until a match is found.
  MR_Next      // Match must occur on the next line.
};

struct TestCase {
  std::string regex;
  int match_rule;

  TestCase(std::string re, int rule = MR_Default)
      : regex(re), match_rule(rule) {}

  void Check(std::stringstream* remaining_output) const {
    benchmark::Regex r;
    std::string err_str;
    r.Init(regex, &err_str);
    CHECK(err_str.empty()) << "Could not construct regex \"" << regex << "\""
                           << " got Error: " << err_str;

    std::string line;
    while (remaining_output->eof() == false) {
      CHECK(remaining_output->good());
      std::getline(*remaining_output, line);
      if (r.Match(line)) return;
      CHECK(match_rule != MR_Next) << "Expected line \"" << line
                                   << "\" to match regex \"" << regex << "\"";
    }

    CHECK(remaining_output->eof() == false)
        << "End of output reached before match for regex \"" << regex
        << "\" was found";
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
  explicit TestReporter(std::vector<benchmark::BenchmarkReporter*> reps)
      : reporters_(reps) {}

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
    for (auto rep : reporters_) rep->ReportRuns(report);
  }

  virtual void Finalize() {
    for (auto rep : reporters_) rep->Finalize();
  }

 private:
  std::vector<benchmark::BenchmarkReporter*> reporters_;
};

int AddCases(std::vector<TestCase>* out,
             std::initializer_list<TestCase> const& v) {
  for (auto const& TC : v) out->push_back(TC);
  return 0;
}

template <class First>
std::string join(First f) {
  return f;
}

template <class First, class... Args>
std::string join(First f, Args&&... args) {
  return std::string(std::move(f)) + "[ ]+" + join(std::forward<Args>(args)...);
}

const char dec_re[] = "[0-9]+\\.[0-9]+";

class ReporterTest {
 public:
  const char* name;
  std::vector<TestCase>* output_cases;
  std::vector<TestCase>* error_cases;
  benchmark::BenchmarkReporter* reporter;
  std::unique_ptr<std::stringstream> out_stream;
  std::unique_ptr<std::stringstream> err_stream;

  ReporterTest(const char* n, std::vector<TestCase>* out_tc,
               std::vector<TestCase>* err_tc, benchmark::BenchmarkReporter* br)
      : name(n),
        output_cases(out_tc),
        error_cases(err_tc),
        reporter(br),
        out_stream(new std::stringstream()),
        err_stream(new std::stringstream()) {
    reporter->SetOutputStream(out_stream.get());
    reporter->SetErrorStream(err_stream.get());
  }
};

std::vector<ReporterTest*> CreateTestCases() {
  return {
      new ReporterTest("ConsoleReporter", &ConsoleOutputTests,
                       &ConsoleErrorTests, new benchmark::ConsoleReporter()),
      new ReporterTest("JSONReporter", &JSONOutputTests, &JSONErrorTests,
                       new benchmark::JSONReporter()),
      new ReporterTest("CSVReporter", &CSVOutputTests, &CSVErrorTests,
                       new benchmark::CSVReporter()),
  };
}

}  // namespace test

#define CONCAT2(x, y) x##y
#define CONCAT(x, y) CONCAT2(x, y)

#define ADD_CASES(...) int CONCAT(dummy, __LINE__) = test::AddCases(__VA_ARGS__)

#endif  // TEST_TEST_REPORTER_H_
