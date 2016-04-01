#include "benchmark/reporter.h"
#include "benchmark/filePath.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <functional>
#include <fstream>
#include <string>
#include <vector>

#include "string_util.h"
#include "walltime.h"

static const char *const benchmarkTitles[4] = {"Time in CPU", "Items per seconds",
                                            "Time in real time",
                                            "Bytes per seconds"};

static const char *const yaxisTitles[4] = {
    "CPU time (items/s)", "Items per second (items/s)", "Real time",
    "Bytes per second (items/s)"};

static const char *const line_toolTipBodies[4] = {
    "Calculated values: <b>{point.key}</b><br/>CPU time: <b>{point.y}</b>",
    "Calculated values: <b>{point.key}</b><br/>Item per time: "
    "<b>{point.y}</b>,",
    "Calculated values: <b>{point.key}</b><br/>Real time: <b>{point.y}</b>",
    "Calculated values: <b>{point.key}</b><br/>Byte per time: "
    "<b>{point.y}byte/s</b>"};

static const char *const column_toolTipBodies[4] = {
    "CPU time: <b>{point.y}</b>", "Item per time: <b>{point.y}item/s</b>",
    "Real time: <b>{point.y}ns</b>",
    "Bytes per second: <b>{point.y}bytes/s</b>"};

static const char *const highChart_Bar_Function =
    "                $('#@BENCHMARK_ID@').highcharts({\n"
    "                    chart: {\n"
    "                        type: 'column',\n"
    "                        zoomType: 'xy'\n"
    "                    },\n"
    "                    title: {\n"
    "                        text: '@BENCHMARK_NAME@'\n"
    "                    },\n"
    "                    legend: {\n"
    "                         enabled: false\n"
    "                    },\n"
    "                    xAxis: {\n"
    "                        categories:[\n@CATEGORIES@"
    "                    ]},\n"
    "                    yAxis: {\n"
    "                        title: {\n"
    "                            text: '@YAXIS_TITLE@'\n"
    "                        },\n  "
    "                        allowDecimals: true\n"
    "                    },\n"
    "                    tooltip: {\n"
    "                        headerFormat: '@TOOLTIP_HEADER@',\n"
    "                        pointFormat: '@TOOLTIP_BODY@',\n"
    "                        borderWidth: 5\n"
    "                    },\n"
    "                    scrollbar: {\n"
    "                        enabled: false\n"
    "                    },\n"
    "                   series:[\n@SERIES@"
    "                   ]\n"
    "                });\n@CHART@";

static const char *const highChart_Line_Function =
    "                $('#@BENCHMARK_ID@').highcharts({\n"
    "                    chart: {\n"
    "                        type: 'line',\n"
    "                        zoomType: 'xy'\n"
    "                    },\n"
    "                    title: {\n"
    "                        text: '@BENCHMARK_NAME@'\n"
    "                    },\n"
    "                    legend: {\n"
    "                         align: 'right',\n"
    "                         verticalAlign: 'top',\n"
    "                         layout: 'vertical'\n"
    "                    },\n"
    "                    xAxis: {\n"
    "                        title: {\n"
    "                              text: 'Amount of calculated values',\n"
    "                              enabled: true\n"
    "                        },\n"
    "                        allowDecimals: true\n"
    "                    },\n"
    "                    yAxis: {\n"
    "                        title: {\n"
    "                            text: '@YAXIS_TITLE@'\n"
    "                        },\n  "
    "                        allowDecimals: true\n"
    "                    },\n"
    "                    tooltip: {\n"
    "                        headerFormat: '@TOOLTIP_HEADER@',\n"
    "                        pointFormat: '@TOOLTIP_BODY@',\n"
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
    "                   series:[\n@SERIES@"
    "                   ]\n"
    "                });\n@CHART@";

static const char *const div_Element =
    "      <div class = \"chart\" id = \"@DIV_NAME@\"></div>\n@DIV@";

static const char *const HTML_Base =
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
    "       <script type = \"text/javascript\">\n"
    "@JQUERY@\n"
    "       </script>\n"
    "       <script type = \"text/javascript\">\n"
    "@HIGHCHART@\n"
    "       </script>\n"
    "       <script type = \"text/javascript\">\n"
    "@HIGHCHART_MORE@\n"
    "       </script>\n"
    "       <script>\n"
    "            $(function () {\n"
    "@CHART@"
    "            });\n"
    "       </script>\n"
    "   </head>\n"
    "   <body>\n"
    "@DIV@"
    "   </body>\n"
    "</html>";

namespace benchmark {
std::string &replaceString(std::string &input, const std::string &from,
                           const std::string &to) {
  size_t position = input.find(from);

  if (position != std::string::npos) {
    input.replace(position, from.size(), to);
  }

  return input;
}

template <typename T>
std::function<std::string(const HTMLReporter::RunData &,
                          const HTMLReporter::RunData &)>
generateErrorbarCallable(T HTMLReporter::RunData::*member) {
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

void outputAllLineCharts(
    std::string &output, const std::array<ChartIt, 4> &lineCharts,
    const std::vector<HTMLReporter::BenchmarkData> &benchmarkTests_Line,
    const std::vector<HTMLReporter::BenchmarkData>
        &benchmarkTests_Line_stddev) {
  const char *benchmarkId[] = {
      "Benchmark_TimeInCpu_Line", "Benchmark_ItemsPerSec_Line",
      "Benchmark_TimeInReal_Line", "Benchmark_BytesPerSec_Line"};
  std::string series;
  std::string chart(highChart_Line_Function);

  for (size_t chartNum = 0; chartNum < 4; chartNum++) {
    for (size_t n = 0; n < benchmarkTests_Line.size(); n++) {
      if (n > 0) {
        series.append(",");
      }

      series.append("{name: '")
          .append(benchmarkTests_Line[n].name)
          .append("',\ndata: [");
      for (size_t m = 0; m < benchmarkTests_Line[n].runData.size(); m++) {
        if (m > 0) {
          series.append(",");
        }
        series.append("[")
            .append(std::to_string(benchmarkTests_Line[n].runData[m].range_x))
            .append(",");
        series.append(
            lineCharts[chartNum].value(benchmarkTests_Line[n].runData[m]));
        series.append("]");
      }
      series.append("]}");

      if (!benchmarkTests_Line_stddev.empty()) {
        series.append(
            ",\n{type: 'errorbar',\nenableMouseTracking: false,\ndata: [");
        for (size_t m = 0; m < benchmarkTests_Line[n].runData.size(); m++) {
          if (m > 0) {
            series.append(",");
          }
          series.append("[")
              .append(std::to_string(benchmarkTests_Line[n].runData[m].range_x))
              .append(",")
              .append(lineCharts[chartNum].error(
                  benchmarkTests_Line[n].runData[m],
                  benchmarkTests_Line_stddev[n].runData[m]));
          series.append("]");
        }
        series.append("]}\n");
      }
    }

    series.append("\n");

    replaceString(chart, "@YAXIS_TITLE@", yaxisTitles[chartNum]);
    replaceString(chart, "@TOOLTIP_BODY@", line_toolTipBodies[chartNum]);
    replaceString(chart, "@BENCHMARK_ID@", benchmarkId[chartNum]);
    replaceString(chart, "@BENCHMARK_NAME@", benchmarkTitles[chartNum]);
    replaceString(chart, "@TOOLTIP_HEADER@",
                  "<b><center>{series.name}</center></b><br/>");
    replaceString(chart, "@SERIES@", series);

    replaceString(output, "@CHART@", chart);
    replaceString(output, "@DIV@", div_Element);
    replaceString(output, "@DIV_NAME@", benchmarkId[chartNum]);

    series.clear();
    chart.assign(highChart_Line_Function);
  }
}

void outputAllBarCharts(
    std::string &output, const std::array<ChartIt, 4> &barCharts,
    const std::vector<HTMLReporter::BenchmarkData> &benchmarkTests_Bar,
    const std::vector<HTMLReporter::BenchmarkData> &benchmarkTests_Bar_stddev) {
  const char *benchmarkId[] = {
      "Benchmark_TimeInCpu_Bar", "Benchmark_ItemsPerSec_Bar",
      "Benchmark_TimeInReal_Bar", "Benchmark_BytesPerSec_Bar"};
  std::string chart(highChart_Bar_Function);
  std::string div(div_Element);
  std::string categories("");
  std::string series("");

  for (size_t chartNum = 0; chartNum < 4; chartNum++) {
    if (benchmarkTests_Bar.size() > 0) {
      categories + "'" + benchmarkTests_Bar[0].name + "'";

      for (size_t n = 1; n < benchmarkTests_Bar.size(); n++) {
        categories + ", '" + benchmarkTests_Bar[n].name + "'";
      }
    }

    series.append("{name: 'Benchmark',\ndata: [\n");
    for (size_t n = 0; n < benchmarkTests_Bar.size(); n++) {
      if (n > 0) {
        series.append(",");
      }
      series.append("['")
          .append(benchmarkTests_Bar[n].name)
          .append("',")
          .append(barCharts[chartNum].value(benchmarkTests_Bar[n].runData[0]))
          .append("]");
    }

    series.append("]}");
    if (!benchmarkTests_Bar_stddev.empty()) {
      series.append(
          ",\n{type: 'errorbar',\nenableMouseTracking: false,\ndata: [");
      for (size_t n = 0; n < benchmarkTests_Bar_stddev.size(); n++) {
        if (n > 0) {
          series.append(",");
        }
        series.append("[")
            .append(barCharts[chartNum].error(
                benchmarkTests_Bar[n].runData[0],
                benchmarkTests_Bar_stddev[n].runData[0]))
            .append("]");
      }
      series.append("]}");
    }
    series.append("\n");

    replaceString(chart, "@YAXIS_TITLE@", yaxisTitles[chartNum]);
    replaceString(chart, "@TOOLTIP_BODY@", column_toolTipBodies[chartNum]);
    replaceString(chart, "@TOOLTIP_HEADER@",
                  "<b><center>{point.key}</center></b><br/>");
    replaceString(div, "@DIV_NAME@", benchmarkId[chartNum]);
    replaceString(chart, "@BENCHMARK_ID@", benchmarkId[chartNum]);
    replaceString(chart, "@BENCHMARK_NAME@", benchmarkTitles[chartNum]);
    replaceString(chart, "@CATEGORIES@", categories);
    replaceString(chart, "@SERIES@", series);
    replaceString(output, "@CHART@", chart);
    replaceString(output, "@DIV@", div);

    categories.clear();
    series.clear();
    chart.assign(highChart_Bar_Function);
    div.assign(div_Element);
  }
}

bool HTMLReporter::ReportContext(const Context &context) {
  std::cerr << "Run on (" << context.num_cpus << " X " << context.mhz_per_cpu
            << " MHz CPU " << ((context.num_cpus > 1) ? "s" : "") << ")\n";

  std::cerr << LocalDateTimeString() << "\n";

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
  std::vector<BenchmarkData> *pBenchmarkTests = nullptr;
  std::vector<BenchmarkData> *pBenchmarkTests_stddev = nullptr;

  if (reports[0].has_arg2) {
    std::cerr << "3D plotting is not available at the moment!\n";
    exit(1);
  }

  // Check if bar or line
  if (reports[0].has_arg1) {
    pBenchmarkTests = &benchmarkTests_Line;
    pBenchmarkTests_stddev = &benchmarkTests_Line_stddev;
  } else {
    pBenchmarkTests = &benchmarkTests_Bar;
    pBenchmarkTests_stddev = &benchmarkTests_Bar_stddev;
  }

  Run reportData = reports[0];

  if (reports.size() >= 2) {
    Run stddevData = reports[0];
    BenchmarkReporter::ComputeStats(reports, &reportData, &stddevData);

    appendRunDataTo(pBenchmarkTests_stddev, stddevData, true);
  }

  appendRunDataTo(pBenchmarkTests, reportData, false);
}

void HTMLReporter::Finalize() {
  std::string output(HTML_Base);
  const std::array<ChartIt, 4> charts = {
      {ChartIt{[](const RunData &mean) { return std::to_string(mean.cpuTime); },
              generateErrorbarCallable(&RunData::cpuTime)},
      ChartIt{
          [](const RunData &mean) { return std::to_string(mean.itemsSecond); },
          generateErrorbarCallable(&RunData::itemsSecond)},
      ChartIt{[](const RunData &mean) { return std::to_string(mean.realTime); },
              generateErrorbarCallable(&RunData::realTime)},
      ChartIt{
          [](const RunData &mean) { return std::to_string(mean.bytesSecond); },
          generateErrorbarCallable(&RunData::bytesSecond)}}};

  outputAllLineCharts(output, charts, benchmarkTests_Line,
                      benchmarkTests_Line_stddev);
  outputAllBarCharts(output, charts, benchmarkTests_Bar,
                     benchmarkTests_Bar_stddev);

  printHTML(std::cout,
            replaceString(replaceString(output, "@CHART@", ""), "@DIV@", ""));
}

void HTMLReporter::writeFile(const char *file) const {
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

std::string HTMLReporter::replaceHTMLSpecialChars(
    const std::string &label) const {
  std::string newLabel(label);

  for (auto pos = newLabel.find_first_of("<'>"); pos != std::string::npos;
       pos = newLabel.find_first_of("<'>", pos)) {
    switch (newLabel[pos]) {
      //< and > can't displayed by highcharts, so they have to be replaced
      case '<':
        newLabel.replace(pos, 1, "⋖");
        break;
      case '>':
        newLabel.replace(pos, 1, "⋗");
        break;
      // For ' it is enough to set a \ before it
      case '\'':
        newLabel.insert(pos, 1, '\\');
        pos++;
        break;
    }
  }

  return newLabel;
}

void HTMLReporter::printHTML(std::ostream &out, const std::string &html) const {
  size_t markerPos = html.find("@JQUERY@");
  size_t stringPos = 0;

  if (markerPos == std::string::npos) {
    std::cerr << "@JQUERY@ marker not found\n";
    return;
  }

  out.write(html.c_str(), markerPos);
  writeFile(JQUERY_PATH);
  stringPos = (markerPos + 8);

  markerPos = html.find("@HIGHCHART@", markerPos);
  if (markerPos == std::string::npos) {
    std::cerr << "@HIGHCHART@ marker not found\n";
    return;
  }

  out.write((html.c_str() + stringPos), (markerPos - stringPos));
  writeFile(HIGHSTOCK_PATH);
  stringPos = (markerPos + 11);

  markerPos = html.find("@HIGHCHART_MORE@", markerPos);
  if (markerPos == std::string::npos) {
    std::cerr << "@HIGHCHART_MORE@ marker not found\n";
    return;
  }

  out.write((html.c_str() + stringPos), (markerPos - stringPos));
  writeFile(HIGHSTOCK_MORE_PATH);
  stringPos = (markerPos + 16);

  out << (html.c_str() + stringPos);
}

void HTMLReporter::appendRunDataTo(std::vector<BenchmarkData> *container,
                                   const Run &data, bool isStddev) const {
  std::string dataName("");

  if (data.has_arg1) {
    dataName = generateInstanceName(
        replaceHTMLSpecialChars(data.benchmark_family), 0, 0, 0, data.min_time,
        data.use_real_time, data.multithreaded, data.threads);

    if (isStddev) {
      dataName.append("_stddev");
    }
  } else {
    dataName = replaceHTMLSpecialChars(data.benchmark_name);
  }

  RunData runData;
  runData.iterations = data.iterations;
  runData.realTime = data.real_accumulated_time;
  runData.cpuTime = data.cpu_accumulated_time;
  runData.bytesSecond = data.bytes_per_second;
  runData.itemsSecond = data.items_per_second;
  runData.range_x = data.arg1;

  std::vector<BenchmarkData>::iterator iter =
      find_if(container->begin(), container->end(),
              [&dataName](const BenchmarkData &value) {
                return value.name == dataName;
              });

  if (iter == container->end()) {
    BenchmarkData benchmarkData;

    benchmarkData.name = std::move(dataName);
    benchmarkData.runData.push_back(runData);
    container->push_back(benchmarkData);
  } else {
    iter->runData.push_back(runData);
  }
}

}  // end namespace benchmark
