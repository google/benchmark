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

enum HTML_Reporter_State { none, label, noLabel };

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
    "                        enabled: true\n"
    "                    },\n"
    "                   series:[\n@SERIES@"
    "                   ]\n"
    "                });\n@CHART@";

const char *highChart_Line_Function =
    "                $('#Benchmark').highcharts({\n"
    "                    chart: {\n"
    "                        type: 'line',\n"
    "                        zoomType: 'xy'\n"
    "                    },\n"
    "                    title: {\n"
    "                        text: 'Benchmark'\n"
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
    "                        allowDecimals: false\n"
    "                    },\n"
    "                    yAxis: {\n"
    "                        title: {\n"
    "                            text: 'time per item (ns)'\n"
    "                        },\n  "
    "                        allowDecimals: false\n"
    "                    },\n"
    "                    tooltip: {\n"
    "                        headerFormat: "
    "'<b><center>{series.name}</center></b><br/>',\n"
    "                        pointFormat: 'Amount of calculated values: "
    "<b>{point.x}</b><br/>Time per item: <b>{point.y}ns</b>',\n"
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

  state = HTML_Reporter_State::none;
  return true;
}

void HTMLReporter::ReportRuns(std::vector<Run> const &reports) {
  size_t subStrPos;
  RunData runData;
  size_t n;
  std::string name;
  std::string tail;

  if (state == HTML_Reporter_State::none) {
    determineState(reports[0].has_arg1);
  }

  name = replaceDefuncChars(reports[0].benchmark_name);

  if (state == HTML_Reporter_State::label) {
    subStrPos = name.find("/");
    if(name.find("/", (subStrPos + 1)) != std::string::npos) {
        name.erase(subStrPos, (name.find("/", subStrPos + 1) - subStrPos));
    }

    else {
        name.erase(subStrPos);
    }
  }

  runData.iterations  = reports[0].iterations;
  runData.realTime    = reports[0].real_accumulated_time;
  runData.cpuTime     = reports[0].cpu_accumulated_time;
  runData.bytesSecond = reports[0].bytes_per_second;
  runData.itemsSecond = reports[0].items_per_second;
  runData.range_x     = reports[0].arg1;

  for (n = 0; n < benchmarkTests.size(); n++) {
    if (benchmarkTests[n].name == name) {
      benchmarkTests[n].runData.push_back(runData);
      break;
    }
  }

  if (n >= benchmarkTests.size()) {
    BenchmarkData benchmarkData;

    benchmarkData.name = std::move(name);
    benchmarkData.runData.push_back(runData);
    benchmarkTests.push_back(benchmarkData);
  }
}

void HTMLReporter::Finalize() {
  if (state == HTML_Reporter_State::label) {
    outputLine();
  }

  else {
    outputColumns();
  }
}

double HTMLReporter::nanoSecondsPerItem(double itemsPerSec) const {
  double dItemsPerSec = itemsPerSec / 1000000000.0;

  return (1.0 / dItemsPerSec);
}

void HTMLReporter::determineState(bool hasX) {
  if (hasX) {
    state = HTML_Reporter_State::label;
  }

  else {
    state = HTML_Reporter_State::noLabel;
  }
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

void HTMLReporter::outputLine() const {
  size_t n = 0;
  size_t m;
  std::string series;
  std::string output(HTML_Base);

  while (n < benchmarkTests.size()) {
    series.append("{name: '")
        .append(benchmarkTests[n].name)
        .append("',\ndata: [");
    m = 0;
    while (m < benchmarkTests[n].runData.size()) {
      series.append("[")
          .append(std::to_string(benchmarkTests[n].runData[m].range_x))
          .append(",")
          .append(std::to_string(
              nanoSecondsPerItem(benchmarkTests[n].runData[m].itemsSecond)))
          .append("]");

      if (++m < benchmarkTests[n].runData.size()) {
        series.append(",");
      }
    }
    series.append("]}");

    if (++n < benchmarkTests.size()) {
      series.append(",");
    }

    series.append("\n");
  }

  replaceString(output, "@CHART@", highChart_Line_Function);
  replaceString(output, "@SERIES@", series);
  replaceString(output, "@DIV@", div_Element);
  replaceString(output, "@DIV_NAME@", "Benchmark");

  printHTML(std::cout, removeCommands(output));
}

void HTMLReporter::outputColumns() const {
  const char *benchmarkId[4] = {"Benchmark_TimeInCpu", "Benchmark_ItemsPerSec",
                                "Benchmark_TimeInReal",
                                "Benchmark_BytesPerSec"};
  const char *benchmarkName[4] = {"Time in CPU", "Items per seconds",
                                  "Time in realtime", "Bytes per second"};

  std::string output(HTML_Base);
  std::string chart(highChart_Column_Function);
  std::string div(div_Element);
  std::string categories;
  std::string series;
  size_t n;
  size_t chartNum = 0;

  do {
    n = 0;
    while (n < benchmarkTests.size()) {
      categories.append("'").append(benchmarkTests[n].name).append("'");
      if (++n < benchmarkTests.size()) {
        categories.append(",");
      }
    }

    replaceString(chart, "@TOOLTIP_HEADER@",
                  "<b><center>{point.key}</center></b><br/>");

    series.append("{name: 'Benchmark',\ndata: [\n");
    n = 0;
    while (n < benchmarkTests.size()) {
      series.append("['").append(benchmarkTests[n].name).append("',");
      switch (chartNum) {
        case 0:
          series.append(std::to_string(benchmarkTests[n].runData[0].cpuTime));
          replaceString(chart, "@YAXIS_TITLE@", "CPU time");
          replaceString(chart, "@TOOLTIP_BODY@", "CPU time: <b>{point.y}</b>");
          break;

        case 1:
          series.append(
              std::to_string(benchmarkTests[n].runData[0].itemsSecond));
          replaceString(chart, "@YAXIS_TITLE@", "Items per second (item/s)");
          replaceString(chart, "@TOOLTIP_BODY@",
                        "Item per time: <b>{point.y}item/s</b>");
          break;

        case 2:
          series.append(std::to_string(benchmarkTests[n].runData[0].realTime));
          replaceString(chart, "@YAXIS_TITLE@", "Real time (ns)");
          replaceString(chart, "@TOOLTIP_BODY@",
                        "Real time: <b>{point.y}ns</b>");
          break;

        case 3:
          series.append(
              std::to_string(benchmarkTests[n].runData[0].bytesSecond));
          replaceString(chart, "@YAXIS_TITLE@", "Bytes per second (byte/s)");
          replaceString(chart, "@TOOLTIP_BODY@",
                        "Bytes per second: <b>{point.y}bytes/s</b>");
          break;
      }

      series.append("]");
      if (++n < benchmarkTests.size()) {
        series.append(",\n");
      }
    }

    series.append("]}\n");

    replaceString(div, "@DIV_NAME@", benchmarkId[chartNum]);
    replaceString(chart, "@BENCHMARK_ID@", benchmarkId[chartNum]);
    replaceString(chart, "@BENCHMARK_NAME@", benchmarkName[chartNum]);
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

  printHTML(std::cout, removeCommands(output));
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
