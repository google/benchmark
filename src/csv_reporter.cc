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

#include <cstdint>
#include <algorithm>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "string_util.h"
#include "walltime.h"

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
  std::ostream& Err = GetErrorStream();
  std::ostream& Out = GetOutputStream();

  Err << "Run on (" << context.num_cpus << " X " << context.mhz_per_cpu
            << " MHz CPU " << ((context.num_cpus > 1) ? "s" : "") << ")\n";

  Err << LocalDateTimeString() << "\n";

  if (context.cpu_scaling_enabled) {
    Err << "***WARNING*** CPU scaling is enabled, the benchmark "
                 "real time measurements may be noisy and will incur extra "
                 "overhead.\n";
  }

#ifndef NDEBUG
  Err << "***WARNING*** Library was built as DEBUG. Timings may be "
               "affected.\n";
#endif
  for (auto B = elements.begin(); B != elements.end(); ) {
    Out << *B++;
    if (B != elements.end())
      Out << ",";
  }
  Out << "\n";
  return true;
}

void CSVReporter::ReportRuns(const std::vector<Run> & reports) {
  if (reports.empty()) {
    return;
  }

  auto error_count = std::count_if(
      reports.begin(), reports.end(),
      [](Run const& run) {return run.error_occurred;});

  std::vector<Run> reports_cp = reports;
  if (reports.size() - error_count >= 2) {
    Run mean_data;
    Run stddev_data;
    BenchmarkReporter::ComputeStats(reports, &mean_data, &stddev_data);
    reports_cp.push_back(mean_data);
    reports_cp.push_back(stddev_data);
  }
  for (auto it = reports_cp.begin(); it != reports_cp.end(); ++it) {
    PrintRunData(*it);
  }
}

void CSVReporter::ReportComplexity(const std::vector<Run>& complexity_reports) {
  if (complexity_reports.size() < 2) {
    // We don't report asymptotic complexity data if there was a single run.
    return;
  }

  Run big_o_data;
  Run rms_data;
  BenchmarkReporter::ComputeBigO(complexity_reports, &big_o_data, &rms_data);

  // Output using PrintRun.
  PrintRunData(big_o_data);
  PrintRunData(rms_data);
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

  double multiplier;
  const char* timeLabel;
  std::tie(timeLabel, multiplier) = GetTimeUnitAndMultiplier(run.time_unit);

  double cpu_time = run.cpu_accumulated_time * multiplier;
  double real_time = run.real_accumulated_time * multiplier;
  if (run.iterations != 0) {
    real_time = real_time / static_cast<double>(run.iterations);
    cpu_time = cpu_time / static_cast<double>(run.iterations);
  }

  // Do not print iteration on bigO and RMS report
  if(!run.report_big_o && !run.report_rms) {
    Out << run.iterations;
  }
  Out << ",";

  Out << real_time << ",";
  Out << cpu_time << ",";

  // Do not print timeLabel on RMS report
  if(!run.report_rms) {
    Out << timeLabel;
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
  Out << ",,"; // for error_occurred and error_message
  Out << '\n';
}

}  // end namespace benchmark
