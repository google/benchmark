#include "benchmark/reporter.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <functional>
#include <fstream>
#include <string>
#include <vector>

#include "benchmark_util.h"
#include "walltime.h"

static const char *const benchmark_titles[4] = {
    "CPU time", "Items per second", "Real time", "Bytes per second"};

static const char *const yaxis_titles[4] = {
    "CPU time [ns]", "Items per second [items/s]", "Real time [ns]",
    "Bytes per second [B/s]"};

static const char *const line_tool_tip_bodies[4] = {
    "Calculated values: <b>{point.key}</b><br/>CPU time: <b>{point.y} "
    "ns</b><br/>Argument 1: <b>{point.x}</b>",
    "Calculated values: <b>{point.key}</b><br/>Items per second: "
    "<b>{point.y}</b><br/>Argument 1: <b>{point.x}</b>",
    "Calculated values: <b>{point.key}</b><br/>Real time: <b>{point.y} "
    "ns</b><br/>Argument 1: <b>{point.x}</b>",
    "Calculated values: <b>{point.key}</b><br/>Bytes per second: "
    "<b>{point.y}</b><br/>Argument 1: <b>{point.x}</b>"};

static const char *const column_tool_tip_bodies[4] = {
    "CPU time: <b>{point.y} ns</b>", "Items per second: <b>{point.y}</b>",
    "Real time: <b>{point.y} ns</b>", "Bytes per second: <b>{point.y}</b>"};

static const char *const high_chart_bar_function =
    "                $('#${BENCHMARK_ID}').highcharts({\n"
    "                    chart: {\n"
    "                        type: 'column',\n"
    "                        zoomType: 'xy'\n"
    "                    },\n"
    "                    title: {\n"
    "                        text: '${BENCHMARK_NAME}'\n"
    "                    },\n"
    "                    legend: {\n"
    "                         enabled: false\n"
    "                    },\n"
    "                    xAxis: {\n"
    "                        categories:[\n${CATEGORIES}"
    "                    ]},\n"
    "                    yAxis: {\n"
    "                        title: {\n"
    "                            text: '${YAXIS_TITLE}'\n"
    "                        },\n  "
    "                        allowDecimals: true\n"
    "                    },\n"
    "                    tooltip: {\n"
    "                        headerFormat: '${TOOLTIP_HEADER}',\n"
    "                        pointFormat: '${TOOLTIP_BODY}',\n"
    "                        borderWidth: 5\n"
    "                    },\n"
    "                    scrollbar: {\n"
    "                        enabled: false\n"
    "                    },\n"
    "                   series:[\n${SERIES}"
    "                   ]\n"
    "                });\n${CHART}";

static const char *const high_chart_line_function =
    "                $('#${BENCHMARK_ID}').highcharts({\n"
    "                    chart: {\n"
    "                        type: 'line',\n"
    "                        zoomType: 'xy'\n"
    "                    },\n"
    "                    title: {\n"
    "                        text: '${BENCHMARK_NAME}'\n"
    "                    },\n"
    "                    legend: {\n"
    "                         align: 'right',\n"
    "                         verticalAlign: 'top',\n"
    "                         layout: 'vertical'\n"
    "                    },\n"
    "                    xAxis: {\n"
    "                        title: {\n"
    "                              text: 'Argument 1',\n"
    "                              enabled: true\n"
    "                        },\n"
    "                        allowDecimals: true\n"
    "                    },\n"
    "                    yAxis: {\n"
    "                        title: {\n"
    "                            text: '${YAXIS_TITLE}'\n"
    "                        },\n  "
    "                        allowDecimals: true\n"
    "                    },\n"
    "                    tooltip: {\n"
    "                        headerFormat: '${TOOLTIP_HEADER}',\n"
    "                        pointFormat: '${TOOLTIP_BODY}',\n"
    "                        borderWidth: 5\n"
    "                    },\n"
    "                    scrollbar: {\n"
    "                        enabled: true\n"
    "                    },\n"
    "                    navigator: {\n"
    "                        enabled: true\n"
    "                    },\n"
    "                    plotOptions: {\n"
    "                        line: {\n"
    "                          marker: {\n"
    "                              enabled: true,\n"
    "                              radius: 3\n"
    "                          },\n"
    "                          lineWidth: 1\n"
    "                        }\n"
    "                    },\n"
    "                   series:[\n${SERIES}"
    "                   ]\n"
    "                });\n${CHART}";

static const char *const div_element =
    "      <div class = \"chart\" id = \"${DIV_NAME}\"></div>\n${DIV}";

static const char *const html_base =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "   <head>\n"
    "       <meta charset = \"UTF-8\"/>\n"
    "       <title>Benchmark</title>\n"
    "       <style type = \"text/css\">\n"
    "           .chart{\n"
    "                height:400px;\n"
    "           }\n"
    "       </style>\n"
    "@CMAKE_JAVASCRIPT_REPLACEMENT@"
    "       <script>\n"
    "            $(function () {\n"
    "${CHART}"
    "            });\n"
    "       </script>\n"
    "   </head>\n"
    "   <body>\n"
    "${DIV}"
    "${CONTEXT}"
    "   </body>\n"
    "</html>";

namespace benchmark {
namespace {
void ReplaceString(std::string *input, const std::string &from,
                   const std::string &to) {
  size_t position = input->find(from);

  if (position != std::string::npos) {
    input->replace(position, from.size(), to);
  }
}

template <typename T>
std::function<std::string(const HTMLReporter::RunData &,
                          const HTMLReporter::RunData &)>
GenerateErrorbarCallable(T HTMLReporter::RunData::*member) {
  return [member](const HTMLReporter::RunData &mean,
                  const HTMLReporter::RunData &stddev) -> std::string {
    T biggerVal = (mean.*member + stddev.*member);
    T shorterVal = (mean.*member - stddev.*member);

    return std::to_string(biggerVal) + ',' + std::to_string(shorterVal);
  };
}

struct ChartIt {
  std::string (*value)(const HTMLReporter::RunData &);
  std::function<std::string(const HTMLReporter::RunData &,
                            const HTMLReporter::RunData &)> error;
};

void OutputAllLineCharts(
    std::string &output, const std::array<ChartIt, 4> &line_charts,
    const std::vector<HTMLReporter::BenchmarkData> &benchmark_tests_line,
    const std::vector<HTMLReporter::BenchmarkData>
        &benchmark_tests_line_stddev) {
  const char *benchmark_id[] = {
      "Benchmark_TimeInCpu_Line", "Benchmark_ItemsPerSec_Line",
      "Benchmark_TimeInReal_Line", "Benchmark_BytesPerSec_Line"};
  std::string series;
  std::string chart(high_chart_line_function);

  for (size_t chart_num = 0; chart_num < 4; chart_num++) {
    for (size_t n = 0; n < benchmark_tests_line.size(); n++) {
      if (n > 0) {
        series.append(",");
      }

      series.append("{name: '")
          .append(benchmark_tests_line[n].name)
          .append("',\ndata: [");
      for (size_t m = 0; m < benchmark_tests_line[n].run_data.size(); m++) {
        if (m > 0) {
          series.append(",");
        }
        series.append("[")
            .append(std::to_string(benchmark_tests_line[n].run_data[m].range_x))
            .append(",");
        series.append(
            line_charts[chart_num].value(benchmark_tests_line[n].run_data[m]));
        series.append("]");
      }
      series.append("]}");

      if (!benchmark_tests_line_stddev.empty()) {
        series.append(
            ",\n{type: 'errorbar',\nenableMouseTracking: false,\ndata: [");
        for (size_t m = 0; m < benchmark_tests_line[n].run_data.size(); m++) {
          if (m > 0) {
            series.append(",");
          }
          series.append("[")
              .append(
                  std::to_string(benchmark_tests_line[n].run_data[m].range_x))
              .append(",")
              .append(line_charts[chart_num].error(
                  benchmark_tests_line[n].run_data[m],
                  benchmark_tests_line_stddev[n].run_data[m]));
          series.append("]");
        }
        series.append("]}\n");
      }
    }

    series.append("\n");

    ReplaceString(&chart, "${YAXIS_TITLE}", yaxis_titles[chart_num]);
    ReplaceString(&chart, "${TOOLTIP_BODY}", line_tool_tip_bodies[chart_num]);
    ReplaceString(&chart, "${BENCHMARK_ID}", benchmark_id[chart_num]);
    ReplaceString(&chart, "${BENCHMARK_NAME}", benchmark_titles[chart_num]);
    ReplaceString(&chart, "${TOOLTIP_HEADER}",
                  "<b><center>{series.name}</center></b><br/>");
    ReplaceString(&chart, "${SERIES}", series);

    ReplaceString(&output, "${CHART}", chart);
    ReplaceString(&output, "${DIV}", div_element);
    ReplaceString(&output, "${DIV_NAME}", benchmark_id[chart_num]);

    series.clear();
    chart.assign(high_chart_line_function);
  }
}

void OutputAllBarCharts(
    std::string &output, const std::array<ChartIt, 4> &bar_charts,
    const std::vector<HTMLReporter::BenchmarkData> &benchmark_tests_bar,
    const std::vector<HTMLReporter::BenchmarkData>
        &benchmark_tests_bar_stddev) {
  const char *benchmark_id[] = {
      "Benchmark_TimeInCpu_Bar", "Benchmark_ItemsPerSec_Bar",
      "Benchmark_TimeInReal_Bar", "Benchmark_BytesPerSec_Bar"};
  std::string chart(high_chart_bar_function);
  std::string div(div_element);
  std::string categories("");
  std::string series("");

  for (size_t chart_num = 0; chart_num < 4; chart_num++) {
    if (benchmark_tests_bar.size() > 0) {
      categories + "'" + benchmark_tests_bar[0].name + "'";

      for (size_t n = 1; n < benchmark_tests_bar.size(); n++) {
        categories + ", '" + benchmark_tests_bar[n].name + "'";
      }
    }

    series.append("{name: 'Benchmark',\ndata: [\n");
    for (size_t n = 0; n < benchmark_tests_bar.size(); n++) {
      if (n > 0) {
        series.append(",");
      }
      series.append("['")
          .append(benchmark_tests_bar[n].name)
          .append("',")
          .append(
              bar_charts[chart_num].value(benchmark_tests_bar[n].run_data[0]))
          .append("]");
    }

    series.append("]}");
    if (!benchmark_tests_bar_stddev.empty()) {
      series.append(
          ",\n{type: 'errorbar',\nenableMouseTracking: false,\ndata: [");
      for (size_t n = 0; n < benchmark_tests_bar_stddev.size(); n++) {
        if (n > 0) {
          series.append(",");
        }
        series.append("[")
            .append(bar_charts[chart_num].error(
                benchmark_tests_bar[n].run_data[0],
                benchmark_tests_bar_stddev[n].run_data[0]))
            .append("]");
      }
      series.append("]}");
    }
    series.append("\n");

    ReplaceString(&chart, "${YAXIS_TITLE}", yaxis_titles[chart_num]);
    ReplaceString(&chart, "${TOOLTIP_BODY}", column_tool_tip_bodies[chart_num]);
    ReplaceString(&chart, "${TOOLTIP_HEADER}",
                  "<b><center>{point.key}</center></b><br/>");
    ReplaceString(&div, "${DIV_NAME}", benchmark_id[chart_num]);
    ReplaceString(&chart, "${BENCHMARK_ID}", benchmark_id[chart_num]);
    ReplaceString(&chart, "${BENCHMARK_NAME}", benchmark_titles[chart_num]);
    ReplaceString(&chart, "${CATEGORIES}", categories);
    ReplaceString(&chart, "${SERIES}", series);
    ReplaceString(&output, "${CHART}", chart);
    ReplaceString(&output, "${DIV}", div);

    categories.clear();
    series.clear();
    chart.assign(high_chart_bar_function);
    div.assign(div_element);
  }
}
}

bool HTMLReporter::ReportContext(const Context &context) {
  context_output.append("<span>")
      .append("Run on (")
      .append(std::to_string(context.num_cpus))
      .append(" X ")
      .append(std::to_string(context.mhz_per_cpu));
  context_output.append(" MHz CPU")
      .append(((context.num_cpus > 1) ? "s" : ""))
      .append(")<br/>");
  context_output.append(LocalDateTimeString()).append("</span>");

  if (context.cpu_scaling_enabled) {
    std::cerr << "***WARNING*** CPU scaling is enabled, the benchmark "
                 "real time measurements may be noisy and will incur extra "
                 "overhead.\n";
  }

#ifndef NDEBUG
  std::cerr << "***WARNING*** Library was built as DEBUG. Timings may be "
               "affected.\n";
#endif
  return true;
}

void HTMLReporter::ReportRuns(std::vector<Run> const &reports) {
  std::vector<BenchmarkData> *benchmark_tests = nullptr;
  std::vector<BenchmarkData> *benchmark_tests_stddev = nullptr;

  if (reports[0].has_arg2) {
    std::clog << "Warning for Benchmark \"" << reports[0].benchmark_name
              << "\": 3D plotting is not implemented! Data will be plotted as "
                 "a chart bar.\n";
  }

  // Check if bar or line
  if (reports[0].has_arg1 && !reports[0].has_arg2) {
    benchmark_tests = &benchmark_tests_line;
    benchmark_tests_stddev = &benchmark_tests_line_stddev;
  } else {
    benchmark_tests = &benchmark_tests_bar;
    benchmark_tests_stddev = &benchmark_tests_bar_stddev;
  }

  Run report_data = reports[0];

  if (reports.size() >= 2) {
    Run stddev_data = reports[0];
    BenchmarkReporter::ComputeStats(reports, &report_data, &stddev_data);

    AppendRunDataTo(benchmark_tests_stddev, stddev_data, true);
  }

  AppendRunDataTo(benchmark_tests, report_data, false);
}

void HTMLReporter::Finalize() {
  std::string output(html_base);
  const std::array<ChartIt, 4> charts = {
      {ChartIt{
           [](const RunData &mean) { return std::to_string(mean.cpu_time); },
           GenerateErrorbarCallable(&RunData::cpu_time)},
       ChartIt{[](const RunData &mean) {
                 return std::to_string(mean.items_second);
               },
               GenerateErrorbarCallable(&RunData::items_second)},
       ChartIt{
           [](const RunData &mean) { return std::to_string(mean.real_time); },
           GenerateErrorbarCallable(&RunData::real_time)},
       ChartIt{[](const RunData &mean) {
                 return std::to_string(mean.bytes_second);
               },
               GenerateErrorbarCallable(&RunData::bytes_second)}}};

  OutputAllLineCharts(output, charts, benchmark_tests_line,
                      benchmark_tests_line_stddev);
  OutputAllBarCharts(output, charts, benchmark_tests_bar,
                     benchmark_tests_bar_stddev);

  ReplaceString(&output, "${CONTEXT}", context_output);
  ReplaceString(&output, "${CHART}", "");
  ReplaceString(&output, "${DIV}", "");

  PrintHTML(std::cout, output);
}

void HTMLReporter::WriteFile(const std::string &file) const {
  std::fstream fin;
  char buffer[256];

  fin.open(file);
  if (fin.is_open()) {
    do {
      fin.read(buffer, 256);
      std::cout.write(buffer, fin.gcount());
    } while (fin.gcount() > 0);

    fin.close();
  }
}

std::string HTMLReporter::ReplaceHTMLSpecialChars(
    const std::string &label) const {
  std::string new_label(label);

  for (auto pos = new_label.find_first_of("<'>"); pos != std::string::npos;
       pos = new_label.find_first_of("<'>", pos)) {
    switch (new_label[pos]) {
      //< and > can't displayed by highcharts, so they have to be replaced
      case '<':
        new_label.replace(pos, 1, "⋖");
        break;
      case '>':
        new_label.replace(pos, 1, "⋗");
        break;
      // For ' it is enough to set a \ before it
      case '\'':
        new_label.insert(pos, 1, '\\');
        pos++;
        break;
    }
  }

  return new_label;
}

void HTMLReporter::PrintHTML(std::ostream &out, const std::string &html) const {
  size_t marker_pos = html.find("${JQUERY}");
  size_t string_pos = 0;

  if (marker_pos != std::string::npos) {
    out.write(html.c_str(), marker_pos);
    WriteFile("@CMAKE_JQUERY_PATH@");
    string_pos = (marker_pos + 9);
  }

  marker_pos = html.find("${HIGHCHART}", marker_pos);

  if (marker_pos != std::string::npos) {
    out.write((html.c_str() + string_pos), (marker_pos - string_pos));
    WriteFile("@CMAKE_HIGHSTOCK_PATH@");
    string_pos = (marker_pos + 12);
  }

  marker_pos = html.find("${HIGHCHART_MORE}", marker_pos);

  if (marker_pos != std::string::npos) {
    out.write((html.c_str() + string_pos), (marker_pos - string_pos));
    WriteFile("@CMAKE_HIGHSTOCK_MORE_PATH@");
    string_pos = (marker_pos + 17);
  }

  out << (html.c_str() + string_pos);
}

void HTMLReporter::AppendRunDataTo(std::vector<BenchmarkData> *container,
                                   const Run &data, bool isStddev) const {
  std::string data_name("");

  if (data.has_arg1 && !data.has_arg2) {
    data_name = benchmark::GenerateInstanceName(
        ReplaceHTMLSpecialChars(data.benchmark_family), 0, 0, 0, data.min_time,
        data.use_real_time, data.multithreaded, data.threads);

    if (isStddev) {
      data_name.append("_stddev");
    }
  } else {
    data_name = ReplaceHTMLSpecialChars(data.benchmark_name);
  }

  RunData run_data;
  double const nano_sec_multiplier = 1e9;
  run_data.iterations = data.iterations;
  run_data.bytes_second = data.bytes_per_second;
  run_data.items_second = data.items_per_second;
  run_data.range_x = data.arg1;
  run_data.real_time = (data.real_accumulated_time * nano_sec_multiplier);
  run_data.cpu_time = (data.cpu_accumulated_time * nano_sec_multiplier);

  if (data.iterations > 1) {
    run_data.real_time /= static_cast<double>(data.iterations);
    run_data.cpu_time /= static_cast<double>(data.iterations);
  }

  std::vector<BenchmarkData>::iterator iter =
      find_if(container->begin(), container->end(),
              [&data_name](const BenchmarkData &value) {
                return value.name == data_name;
              });

  if (iter == container->end()) {
    BenchmarkData benchmark_data;

    benchmark_data.name = std::move(data_name);
    benchmark_data.run_data.push_back(run_data);
    container->push_back(benchmark_data);
  } else {
    iter->run_data.push_back(run_data);
  }
}
}  // end namespace benchmark
