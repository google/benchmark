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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "check.h"
#include "colorprint.h"
#include "stat.h"
#include "string_util.h"
#include "walltime.h"

namespace benchmark {
namespace {

void ComputeStats(const std::vector<BenchmarkReporter::Run>& reports,
                  BenchmarkReporter::Run* mean_data,
                  BenchmarkReporter::Run* stddev_data) {
  CHECK(reports.size() >= 2) << "Cannot compute stats for less than 2 reports";
  // Accumulators.
  Stat1_d real_accumulated_time_stat;
  Stat1_d cpu_accumulated_time_stat;
  Stat1_d bytes_per_second_stat;
  Stat1_d items_per_second_stat;
  // All repetitions should be run with the same number of iterations so we
  // can take this information from the first benchmark.
  std::size_t const run_iterations = reports.front().iterations;

  // Populate the accumulators.
  for (BenchmarkReporter::Run const& run : reports) {
    CHECK_EQ(reports[0].benchmark_name, run.benchmark_name);
    CHECK_EQ(run_iterations, run.iterations);
    real_accumulated_time_stat +=
        Stat1_d(run.real_accumulated_time/run.iterations, run.iterations);
    cpu_accumulated_time_stat +=
        Stat1_d(run.cpu_accumulated_time/run.iterations, run.iterations);
    items_per_second_stat += Stat1_d(run.items_per_second, run.iterations);
    bytes_per_second_stat += Stat1_d(run.bytes_per_second, run.iterations);
  }

  // Get the data from the accumulator to BenchmarkReporter::Run's.
  mean_data->benchmark_name = reports[0].benchmark_name + "_mean";
  mean_data->iterations = run_iterations;
  mean_data->real_accumulated_time = real_accumulated_time_stat.Mean() *
                                     run_iterations;
  mean_data->cpu_accumulated_time = cpu_accumulated_time_stat.Mean() *
                                    run_iterations;
  mean_data->bytes_per_second = bytes_per_second_stat.Mean();
  mean_data->items_per_second = items_per_second_stat.Mean();

  // Only add label to mean/stddev if it is same for all runs
  mean_data->report_label = reports[0].report_label;
  for (std::size_t i = 1; i < reports.size(); i++) {
    if (reports[i].report_label != reports[0].report_label) {
      mean_data->report_label = "";
      break;
    }
  }

  stddev_data->benchmark_name = reports[0].benchmark_name + "_stddev";
  stddev_data->report_label = mean_data->report_label;
  stddev_data->iterations = 0;
  stddev_data->real_accumulated_time =
      real_accumulated_time_stat.StdDev();
  stddev_data->cpu_accumulated_time =
      cpu_accumulated_time_stat.StdDev();
  stddev_data->bytes_per_second = bytes_per_second_stat.StdDev();
  stddev_data->items_per_second = items_per_second_stat.StdDev();
}

} // end namespace

void BenchmarkReporter::Finalize() {
}

BenchmarkReporter::~BenchmarkReporter() {
}

bool ConsoleReporter::ReportContext(const Context& context) {
  name_field_width_ = context.name_field_width;

  fprintf(stdout,
          "Run on (%d X %0.0f MHz CPU%s)\n",
          context.num_cpus,
          context.mhz_per_cpu,
          (context.num_cpus > 1) ? "s" : "");

  int remainder_us;
  std::string walltime_str = walltime::Print(
                                walltime::Now(), "%Y/%m/%d-%H:%M:%S",
                                true,  // use local timezone
                                &remainder_us);
  fprintf(stdout, "%s\n", walltime_str.c_str());

  if (context.cpu_scaling_enabled) {
    fprintf(stdout, "***WARNING*** CPU scaling is enabled, the benchmark "
                    "timings may be noisy\n");
  }

#ifndef NDEBUG
  fprintf(stdout, "Build Type: DEBUG\n");
#endif

  int output_width =
      fprintf(stdout,
              "%-*s %10s %10s %10s\n",
              static_cast<int>(name_field_width_),
              "Benchmark",
              "Time(ns)", "CPU(ns)",
              "Iterations");
  fprintf(stdout, "%s\n", std::string(output_width - 1, '-').c_str());

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

  if (reports.size() < 2) {
    // We don't report aggregated data if there was a single run.
    return;
  }

  Run mean_data;
  Run stddev_data;
  ComputeStats(reports, &mean_data, &stddev_data);

  // Output using PrintRun.
  PrintRunData(mean_data);
  PrintRunData(stddev_data);
  fprintf(stdout, "\n");
}

void ConsoleReporter::PrintRunData(const Run& result) {
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

  double const multiplier = 1e9; // nano second multiplier
  ColorPrintf(COLOR_GREEN, "%-*s ",
              name_field_width_, result.benchmark_name.c_str());
  if (result.iterations == 0) {
    ColorPrintf(COLOR_YELLOW, "%10.0f %10.0f ",
                result.real_accumulated_time * multiplier,
                result.cpu_accumulated_time * multiplier);
  } else {
    ColorPrintf(COLOR_YELLOW, "%10.0f %10.0f ",
                (result.real_accumulated_time * multiplier) /
                    (static_cast<double>(result.iterations)),
                (result.cpu_accumulated_time * multiplier) /
                    (static_cast<double>(result.iterations)));
  }
  ColorPrintf(COLOR_CYAN, "%10lld", result.iterations);
  ColorPrintf(COLOR_DEFAULT, "%*s %*s %s\n",
              13, rate.c_str(),
              18, items.c_str(),
              result.report_label.c_str());
}

namespace {

std::string FormatKV(std::string const& key, std::string const& value) {
  return StringPrintF("\"%s\": \"%s\"", key.c_str(), value.c_str());
}

std::string FormatKV(std::string const& key, const char* value) {
  return StringPrintF("\"%s\": \"%s\"", key.c_str(), value);
}

std::string FormatKV(std::string const& key, bool value) {
  return StringPrintF("\"%s\": %s", key.c_str(), value ? "true" : "false");
}

std::string FormatKV(std::string const& key, int64_t value) {
  std::stringstream ss;
  ss << '"' << key << "\": " << value;
  return ss.str();
}

std::string FormatKV(std::string const& key, std::size_t value) {
  std::stringstream ss;
  ss << '"' << key << "\": " << value;
  return ss.str();
}

int64_t RoundDouble(double v) {
    return static_cast<int64_t>(v + 0.5);
}

} // end namespace

bool JSONReporter::ReportContext(const Context& context) {
  std::ostream& out = std::cout;

  out << "{\n";
  std::string inner_indent(2, ' ');

  // Open context block and print context information.
  out << inner_indent << "\"context\": {\n";
  std::string indent(4, ' ');
  int remainder_us;
  std::string walltime_value = walltime::Print(
                                    walltime::Now(), "%Y/%m/%d-%H:%M:%S",
                                    true,  // use local timezone
                                    &remainder_us);
  out << indent << FormatKV("date", walltime_value) << ",\n";

  out << indent
      << FormatKV("num_cpus", static_cast<int64_t>(context.num_cpus))
      << ",\n";
  out << indent
      << FormatKV("mhz_per_cpu", RoundDouble(context.mhz_per_cpu))
      << ",\n";
  out << indent
      << FormatKV("cpu_scaling_enabled", context.cpu_scaling_enabled)
      << ",\n";

#if defined(NDEBUG)
  const char* build_type = "release";
#else
  const char* build_type = "debug";
#endif
  out << indent << FormatKV("build_type", build_type) << "\n";
  // Close context block and open the list of benchmarks.
  out << inner_indent << "},\n";
  out << inner_indent << "\"benchmarks\": [\n";
  return true;
}

void JSONReporter::ReportRuns(std::vector<Run> const& reports) {
  if (reports.empty()) {
    return;
  }
  std::string indent(4, ' ');
  std::ostream& out = std::cout;
  if (!first_report_) {
    out << ",\n";
  }
  first_report_ = false;
  std::vector<Run> reports_cp = reports;
  if (reports.size() >= 2) {
    Run mean_data;
    Run stddev_data;
    ComputeStats(reports, &mean_data, &stddev_data);
    reports_cp.push_back(mean_data);
    reports_cp.push_back(stddev_data);
  }
  for (auto it = reports_cp.begin(); it != reports_cp.end(); ++it) {
     out << indent << "{\n";
     PrintRunData(*it);
     out << indent << '}';
     auto it_cp = it;
     if (++it_cp != reports_cp.end()) {
         out << ',';
     }
  }
}

void JSONReporter::Finalize() {
    // Close the list of benchmarks and the top level object.
    std::cout << "\n  ]\n}\n";
}

void JSONReporter::PrintRunData(Run const& run) {
    double const multiplier = 1e9; // nano second multiplier
    double cpu_time = run.cpu_accumulated_time * multiplier;
    double real_time = run.real_accumulated_time * multiplier;
    if (run.iterations != 0) {
        real_time = real_time / static_cast<double>(run.iterations);
        cpu_time = cpu_time / static_cast<double>(run.iterations);
    }

    std::string indent(6, ' ');
    std::ostream& out = std::cout;
    out << indent
        << FormatKV("name", run.benchmark_name)
        << ",\n";
    out << indent
        << FormatKV("iterations", run.iterations)
        << ",\n";
    out << indent
        << FormatKV("real_time", RoundDouble(real_time))
        << ",\n";
    out << indent
        << FormatKV("cpu_time", RoundDouble(cpu_time));
    if (run.bytes_per_second > 0.0) {
        out << ",\n" << indent
            << FormatKV("bytes_per_second", RoundDouble(run.bytes_per_second));
    }
    if (run.items_per_second > 0.0) {
        out << ",\n" << indent
            << FormatKV("items_per_second", RoundDouble(run.items_per_second));
    }
    if (!run.report_label.empty()) {
        out << ",\n" << indent
            << FormatKV("label", run.report_label);
    }
    out << '\n';
}

} // end namespace benchmark
