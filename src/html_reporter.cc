#include "benchmark/reporter.h"
#include "benchmark/filePath.h"

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include "string_util.h"
#include "walltime.h"

std::string &replaceString(std::string &input, const std::string &from,
                           const std::string &to) {
  size_t position = input.find(from);

  if (position != std::string::npos) {
    input.replace(position, from.size(), to);
  }

  return input;
}

std::string copyStringFrom(const std::string &input, const std::string &begin,
                           const std::string &end) {
  size_t startPos = input.find(begin);
  size_t endPos = input.find(end);

  if (startPos == std::string::npos || endPos == std::string::npos) {
    return std::string();
  }

  startPos = (startPos + begin.size());

  return std::string(input, startPos, (endPos - startPos));
}

bool IsZero(double n) {
    return std::abs(n) < std::numeric_limits<double>::epsilon();
}

const char *highChart_Column_Function =
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

const char *highChart_Line_Function =
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

const char *div_Element =
    "      <div class = \"chart\" id = \"@DIV_NAME@\"></div>\n@DIV@";

const char *HTML_Base =
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

const char *HTMLReporter::benchmarkName[] = {
    "Time in CPU", "Items per seconds", "Time in realtime", "Bytes per second"};

HTMLReporter::HTMLReporter(const std::string &nUserString)
    : userString(nUserString) {}

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
  std::vector<BenchmarkData> *aktContainer;
  RunData runData;
  size_t n;
  std::string name;
  std::string tail;

  if (reports[0].has_arg1) {
    aktContainer = &this->benchmarkTests_Line;

    name = generateInstanceName(replaceDefuncChars(reports[0].benchmark_family), 0, 0, 0, reports[0].min_time, reports[0].use_real_time, reports[0].multithreaded, reports[0].threads);
  }

  else {
    aktContainer = &this->benchmarkTests_Column;
    name = replaceDefuncChars(reports[0].benchmark_name);
  }

  runData.iterations = reports[0].iterations;
  runData.realTime = reports[0].real_accumulated_time;
  runData.cpuTime = reports[0].cpu_accumulated_time;
  runData.bytesSecond = reports[0].bytes_per_second;
  runData.itemsSecond = reports[0].items_per_second;
  runData.range_x = reports[0].arg1;

  for (n = 0; n < aktContainer->size(); n++) {
    if ((*aktContainer)[n].name == name) {
      (*aktContainer)[n].runData.push_back(runData);
      break;
    }
  }

  if (n >= aktContainer->size()) {
    BenchmarkData benchmarkData;

    benchmarkData.name = std::move(name);
    benchmarkData.runData.push_back(runData);
    aktContainer->push_back(benchmarkData);
  }
}

void HTMLReporter::Finalize() {
  std::string output(HTML_Base);

  outputLine(output);
  outputColumns(output);

  printHTML(std::cout, removeCommands(output));
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

std::string HTMLReporter::replaceDefuncChars(const std::string &label) {
  size_t charPos;
  std::string newLabel(label);

  // Tempaltes greats < >, which must be removed
  charPos = newLabel.find('<');
  while (charPos != std::string::npos) {
    newLabel[charPos] = '(';
    charPos = newLabel.find('<', (charPos + 1));
  }

  charPos = newLabel.find('>');
  while (charPos != std::string::npos) {
    newLabel[charPos] = ')';
    charPos = newLabel.find('>', (charPos + 1));
  }

  return newLabel;
}

void HTMLReporter::outputLine(std::string &output) const {
  const char *benchmarkId[] = {
      "Benchmark_TimeInCpu_Line", "Benchmark_ItemsPerSec_Line",
      "Benchmark_TimeInReal_Line", "Benchmark_BytesPerSec_Line"};
  size_t n = 0;
  size_t m;
  size_t chartNum = 0;
  std::string series;
  std::string chart(highChart_Line_Function);

  do {
    replaceString(chart, "@BENCHMARK_ID@", benchmarkId[chartNum]);
    replaceString(chart, "@BENCHMARK_NAME@",
                  HTMLReporter::benchmarkName[chartNum]);
    replaceString(chart, "@TOOLTIP_HEADER@",
                  "<b><center>{series.name}</center></b><br/>");

    n = 0;
    while (n < benchmarkTests_Line.size()) {
      series.append("{name: '")
          .append(benchmarkTests_Line[n].name)
          .append("',\ndata: [");
      m = 0;
      while (m < benchmarkTests_Line[n].runData.size()) {
        series.append("[")
            .append(std::to_string(benchmarkTests_Line[n].runData[m].range_x))
            .append(",");
        switch (chartNum) {
          case 0:
            series.append(
                std::to_string(benchmarkTests_Line[n].runData[m].cpuTime));
            replaceString(chart, "@YAXIS_TITLE@", "CPU time (items/s)");
            replaceString(chart, "@TOOLTIP_BODY@",
                          "Calculated values: <b>{point.key}</b><br/>CPU time: "
                          "<b>{point.y}</b>");
            break;

          case 1:
            series.append(
                std::to_string(benchmarkTests_Line[n].runData[m].itemsSecond));
            replaceString(chart, "@YAXIS_TITLE@", "Items per second (items/s)");
            replaceString(chart, "@TOOLTIP_BODY@",
                          "Calculated values: <b>{point.key}</b><br/>Item per "
                          "time: <b>{point.y}</b>");
            break;

          case 2:
            series.append(
                std::to_string(benchmarkTests_Line[n].runData[m].realTime));
            replaceString(chart, "@YAXIS_TITLE@", "Real time");
            replaceString(chart, "@TOOLTIP_BODY@",
                          "Calculated values: <b>{point.key}</b><br/>Real "
                          "time: <b>{point.y}</b>");
            break;

          case 3:
            series.append(
                std::to_string(benchmarkTests_Line[n].runData[m].bytesSecond));
            replaceString(chart, "@YAXIS_TITLE@", "Bytes per second (items/s)");
            replaceString(chart, "@TOOLTIP_BODY@",
                          "Calculated values: <b>{point.key}</b><br/>Byte per "
                          "time: <b>{point.y}byte/s</b>");
            break;
        }
        series.append("]");

        if (++m < benchmarkTests_Line[n].runData.size()) {
          series.append(",");
        }
      }
      series.append("]}");

      if (++n < benchmarkTests_Line.size()) {
        series.append(",");
      }

      series.append("\n");
    }
    replaceString(chart, "@SERIES@", series);

    replaceString(output, "@CHART@", chart);
    replaceString(output, "@DIV@", div_Element);
    replaceString(output, "@DIV_NAME@", benchmarkId[chartNum]);

    series.clear();
    chart.assign(highChart_Line_Function);

    chartNum++;
  } while (chartNum < 4);
}

void HTMLReporter::outputColumns(std::string &output) const {
  const char *benchmarkId[] = {
      "Benchmark_TimeInCpu_Column", "Benchmark_ItemsPerSec_Column",
      "Benchmark_TimeInReal_Column", "Benchmark_BytesPerSec_Column"};
  std::string chart(highChart_Column_Function);
  std::string div(div_Element);
  std::string categories;
  std::string series;
  size_t n;
  size_t chartNum = 0;

  do {
    n = 0;
    while (n < benchmarkTests_Column.size()) {
      categories.append("'").append(benchmarkTests_Column[n].name).append("'");
      if (++n < benchmarkTests_Column.size()) {
        categories.append(",");
      }
    }

    replaceString(chart, "@TOOLTIP_HEADER@",
                  "<b><center>{point.key}</center></b><br/>");

    series.append("{name: 'Benchmark',\ndata: [\n");
    n = 0;
    while (n < benchmarkTests_Column.size()) {
      series.append("['").append(benchmarkTests_Column[n].name).append("',");
      switch (chartNum) {
        case 0:
          series.append(
              std::to_string(benchmarkTests_Column[n].runData[0].cpuTime));
          replaceString(chart, "@YAXIS_TITLE@", "CPU time");
          replaceString(chart, "@TOOLTIP_BODY@", "CPU time: <b>{point.y}</b>");
          break;

        case 1:
          series.append(
              std::to_string(benchmarkTests_Column[n].runData[0].itemsSecond));
          replaceString(chart, "@YAXIS_TITLE@", "Items per second (item/s)");
          replaceString(chart, "@TOOLTIP_BODY@",
                        "Item per time: <b>{point.y}item/s</b>");
          break;

        case 2:
          series.append(
              std::to_string(benchmarkTests_Column[n].runData[0].realTime));
          replaceString(chart, "@YAXIS_TITLE@", "Real time (ns)");
          replaceString(chart, "@TOOLTIP_BODY@",
                        "Real time: <b>{point.y}ns</b>");
          break;

        case 3:
          series.append(
              std::to_string(benchmarkTests_Column[n].runData[0].bytesSecond));
          replaceString(chart, "@YAXIS_TITLE@", "Bytes per second (byte/s)");
          replaceString(chart, "@TOOLTIP_BODY@",
                        "Bytes per second: <b>{point.y}bytes/s</b>");
          break;
      }

      series.append("]");
      if (++n < benchmarkTests_Column.size()) {
        series.append(",\n");
      }
    }

    series.append("]}\n");

    replaceString(div, "@DIV_NAME@", benchmarkId[chartNum]);
    replaceString(chart, "@BENCHMARK_ID@", benchmarkId[chartNum]);
    replaceString(chart, "@BENCHMARK_NAME@",
                  HTMLReporter::benchmarkName[chartNum]);
    replaceString(chart, "@CATEGORIES@", categories);
    replaceString(chart, "@SERIES@", series);
    replaceString(output, "@CHART@", chart);
    replaceString(output, "@DIV@", div);

    categories.clear();
    series.clear();
    chart.assign(highChart_Column_Function);
    div.assign(div_Element);
    chartNum++;
  } while (chartNum < 4);
}

std::string &HTMLReporter::removeCommands(std::string &data) const {
  return replaceString(replaceString(data, "@CHART@", ""), "@DIV@", "");
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

  markerPos = html.find("@HIGHCHART@");
  if (markerPos == std::string::npos) {
    std::cerr << "@HIGHCHART@ marker not found\n";
    return;
  }

  out.write((html.c_str() + stringPos), (markerPos - stringPos));
  writeFile(HIGHSTOCK_PATH);
  stringPos = (markerPos + 11);

  out << (html.c_str() + stringPos);
}

}  // end namespace benchmark
