// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "benchmark/reporter.h"
#include "complexity.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "string_util.h"
#include "walltime.h"

#include <set>

// File format reference: http://edoceo.com/utilitas/csv-file-format.

namespace benchmark {

namespace {
std::vector<std::string> elements = {
  "name",
  "iterations",
  "real_time",
  "cpu_time",
  "time_unit",
  "bytes_per_second",
  "items_per_second",
  "label",
  "error_occurred",
  "error_message"
};
}

bool CSVReporter::ReportContext(const Context& context) {
  PrintBasicContext(&GetErrorStream(), context);
  return true;
}

void CSVReporter::ReportRuns(const std::vector<Run> & reports) {

  // find the names of all the user counters
  std::set< std::string > user_counter_names;
  for (const auto& run : reports) {
    for (const auto& cnt : run.counters) {
      user_counter_names.insert(cnt.Name());
    }
  }

  // print the header
  std::ostream& Out = GetOutputStream();
  for (auto B = elements.begin(); B != elements.end(); ) {
    Out << *B++;
    if (B != elements.end())
      Out << ",";
  }
  for (const auto& name : user_counter_names) {
    Out << "," << name;
  }
  Out << "\n";

  // print results for each run
  for (const auto& run : reports) {
    PrintRunData(run);

    // Print user counters
    // <jppm> .... this should be done in PrintRunData(), but that would require
    // storing user_counter_names either in the anon namespace above
    // or as a member in the CSVReporterClass, which in turn would
    // #include <set> in the public reporter.h header. Passing it as an argument
    // would also require the include.
    // So I'll defer judgment here.
    for (const auto &name : user_counter_names) {
      Out << ",";
      if(run.counters.Exists(name)) {
        Out << run.counters.Get(name).Value();
      }
    }
    Out << '\n';
  }

}

void CSVReporter::PrintRunData(const Run & run) {
  std::ostream& Out = GetOutputStream();

  // Field with embedded double-quote characters must be doubled and the field
  // delimited with double-quotes.
  std::string name = run.benchmark_name;
  ReplaceAll(&name, "\"", "\"\"");
  Out << '"' << name << "\",";
  if (run.error_occurred) {
    Out << std::string(elements.size() - 3, ',');
    Out << "true,";
    std::string msg = run.error_message;
    ReplaceAll(&msg, "\"", "\"\"");
    Out << '"' << msg << "\"\n";
    return;
  }

  // Do not print iteration on bigO and RMS report
  if (!run.report_big_o && !run.report_rms) {
    Out << run.iterations;
  }
  Out << ",";

  Out << run.GetAdjustedRealTime() << ",";
  Out << run.GetAdjustedCPUTime() << ",";

  // Do not print timeLabel on bigO and RMS report
  if (run.report_big_o) {
    Out << GetBigOString(run.complexity);
  } else if (!run.report_rms) {
    Out << GetTimeUnitString(run.time_unit);
  }
  Out << ",";

  if (run.bytes_per_second > 0.0) {
    Out << run.bytes_per_second;
  }
  Out << ",";
  if (run.items_per_second > 0.0) {
    Out << run.items_per_second;
  }
  Out << ",";
  if (!run.report_label.empty()) {
    // Field with embedded double-quote characters must be doubled and the field
    // delimited with double-quotes.
    std::string label = run.report_label;
    ReplaceAll(&label, "\"", "\"\"");
    Out << "\"" << label << "\"";
  }
  Out << ",,";  // for error_occurred and error_message

}

}  // end namespace benchmark
