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

#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "check.h"
#include "colorprint.h"
#include "commandlineflags.h"
#include "internal_macros.h"
#include "string_util.h"
#include "walltime.h"

namespace benchmark {

bool ConsoleReporter::ReportContext(const Context& context) {
  name_field_width_ = context.name_field_width;

  auto& Out = GetOutputStream();
  auto& Err = GetErrorStream();

#ifdef BENCHMARK_OS_WINDOWS
  if (FLAGS_color_print && &Out != &std::cout) {
      Err << "Color printing is only supported for stdout on windows. "
              "Disabling color printing\n";
      FLAGS_color_print = false;
  }
#endif

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
  std::string str = FormatString("%-*s %13s %13s %10s\n",
                             static_cast<int>(name_field_width_), "Benchmark",
                             "Time", "CPU", "Iterations");
  Out << str << std::string(str.length() - 1, '-') << "\n";

  return true;
}

void ConsoleReporter::ReportRuns(const std::vector<Run>& reports) {
  if (reports.empty()) {
    return;
  }

  for (Run const& run : reports) {
    CHECK_EQ(reports[0].benchmark_name, run.benchmark_name);
    PrintRunData(run);
  }

  auto error_count = std::count_if(
      reports.begin(), reports.end(),
      [](Run const& run) {return run.error_occurred;});

  if (reports.size() - error_count < 2) {
    // We don't report aggregated data if there was a single run.
    return;
  }

  Run mean_data;
  Run stddev_data;
  BenchmarkReporter::ComputeStats(reports, &mean_data, &stddev_data);

  // Output using PrintRun.
  PrintRunData(mean_data);
  PrintRunData(stddev_data);
}

void ConsoleReporter::ReportComplexity(const std::vector<Run> & complexity_reports) {
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

void ConsoleReporter::PrintRunData(const Run& result) {
  auto& Out = GetOutputStream();

  auto name_color = (result.report_big_o || result.report_rms)
      ? COLOR_BLUE : COLOR_GREEN;
  ColorPrintf(Out, name_color, "%-*s ", name_field_width_,
              result.benchmark_name.c_str());

  if (result.error_occurred) {
    ColorPrintf(Out, COLOR_RED, "ERROR OCCURRED: \'%s\'",
                result.error_message.c_str());
    ColorPrintf(Out, COLOR_DEFAULT, "\n");
    return;
  }
  // Format bytes per second
  std::string rate;
  if (result.bytes_per_second > 0) {
    rate = StrCat(" ", HumanReadableNumber(result.bytes_per_second), "B/s");
  }

  // Format items per second
  std::string items;
  if (result.items_per_second > 0) {
    items = StrCat(" ", HumanReadableNumber(result.items_per_second),
                   " items/s");
  }

  double multiplier;
  const char* timeLabel;
  std::tie(timeLabel, multiplier) = GetTimeUnitAndMultiplier(result.time_unit);

  if(result.report_big_o) {
    std::string big_o = result.report_big_o ? GetBigOString(result.complexity) : "";
    ColorPrintf(Out, COLOR_YELLOW, "%10.4f %s %10.4f %s ",
                result.real_accumulated_time * multiplier,
                big_o.c_str(),
                result.cpu_accumulated_time * multiplier,
                big_o.c_str());
  } else if(result.report_rms) {
    ColorPrintf(Out, COLOR_YELLOW, "%10.0f %% %10.0f %% ",
                result.real_accumulated_time * multiplier * 100,
                result.cpu_accumulated_time * multiplier * 100);
  } else if (result.iterations == 0) {

    ColorPrintf(Out, COLOR_YELLOW, "%10.0f %s %10.0f %s ",
                result.real_accumulated_time * multiplier,
                timeLabel,
                result.cpu_accumulated_time * multiplier,
                timeLabel);
  } else {
    ColorPrintf(Out, COLOR_YELLOW, "%10.0f %s %10.0f %s ",
                (result.real_accumulated_time * multiplier) /
                    (static_cast<double>(result.iterations)),
                timeLabel,
                (result.cpu_accumulated_time * multiplier) /
                    (static_cast<double>(result.iterations)),
                timeLabel);
  }

  if(!result.report_big_o && !result.report_rms) {
    ColorPrintf(Out, COLOR_CYAN, "%10lld", result.iterations);
  }

  if (!rate.empty()) {
    ColorPrintf(Out, COLOR_DEFAULT, " %*s", 13, rate.c_str());
  }

  if (!items.empty()) {
    ColorPrintf(Out, COLOR_DEFAULT, " %*s", 18, items.c_str());
  }

  if (!result.report_label.empty()) {
    ColorPrintf(Out, COLOR_DEFAULT, " %s", result.report_label.c_str());
  }

  ColorPrintf(Out, COLOR_DEFAULT, "\n");
}

}  // end namespace benchmark
