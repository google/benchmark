#include "benchmark/reporter.h"
#include "benchmark/filePath.h"

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <array>
#include <functional>

#include "string_util.h"
#include "walltime.h"

static const char benchmarkTitles[4][18] = {
"Time in CPU",
"Items per seconds",
"Time in real time",
"Bytes per seconds"
};

static const char yaxisTitles[4][29] = {
"CPU time (items/s)",
"Items per second (items/s)",
"Real time",
"Bytes per second (items/s)"
};

static const char line_toolTipBodies[4][80] = {
"Calculated values: <b>{point.key}</b><br/>CPU time: b>{point.y}</b>",
"Calculated values: <b>{point.key}</b><br/>Item per time: <b>{point.y}</b>,",
"Calculated values: <b>{point.key}</b><br/>Real time: <b>{point.y}</b>",
"Calculated values: <b>{point.key}</b><br/>Byte per time: <b>{point.y}byte/s</b>"
};

static const char column_toolTipBodies[4][42] = {
"CPU time: <b>{point.y}</b>",
"Item per time: <b>{point.y}item/s</b>",
"Real time: <b>{point.y}ns</b>",
"Bytes per second: <b>{point.y}bytes/s</b>"
};

//Pull-Request: "Wo soll ich (replaceString) hin?!"
std::string &replaceString(std::string &input, const std::string &from,
                           const std::string &to) {
  size_t position = input.find(from);

  if (position != std::string::npos) {
    input.replace(position, from.size(), to);
  }

  return input;
}

static const char *highChart_Column_Function =
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

static const char *highChart_Line_Function =
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

static const char *div_Element =
    "      <div class = \"chart\" id = \"@DIV_NAME@\"></div>\n@DIV@";

static const char *HTML_Base =
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

template<typename T>
std::function<std::string(const HTMLReporter::RunData &, const HTMLReporter::RunData &)> generateErrorbarCallable(T HTMLReporter::RunData::* member) {
return [member](const HTMLReporter::RunData &mean, const HTMLReporter::RunData &stddev) {
        T biggerVal = (mean.*member + stddev.*member);
        T shorterVal = (mean.*member - stddev.*member);

        return std::to_string(biggerVal) + ',' + std::to_string(shorterVal);
    };
}

struct ChartIt {
    std::string (*value)(const HTMLReporter::RunData &);
    std::function<std::string(const HTMLReporter::RunData &, const HTMLReporter::RunData &)> error;
};

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
  Run workingRun, stddevRun;
  RunData runData;
  size_t n;
  std::string name;
  int stddevState = 0;

  workingRun = reports[0];
  stddevRun  = reports[0];

  if(reports.size() >= 2) {
    BenchmarkReporter::ComputeStats(reports, &workingRun, &stddevRun);
    stddevState = 1;
  }

  do {
      if(workingRun.has_arg2) {
        std::cerr << "3D plotting is not available at the moment!\n";
        exit(1);
      }

      if (workingRun.has_arg1) {
        name = generateInstanceName(replaceDefuncChars(workingRun.benchmark_family), 0, 0, 0, workingRun.min_time, workingRun.use_real_time, workingRun.multithreaded, workingRun.threads);

        if(stddevState == 2) {
            name += "_stddev";
            aktContainer = &this->benchmarkTests_Line_stddev;
            stddevState = 0;
        }

        else {
            aktContainer = &this->benchmarkTests_Line;
        }
      }

      else {
        name = replaceDefuncChars(workingRun.benchmark_name);
        if(stddevState == 2) {
            aktContainer = &this->benchmarkTests_Column_stddev;
            stddevState = 0;
        }

        else {
            aktContainer = &this->benchmarkTests_Column;
        }
      }

      runData.iterations = workingRun.iterations;
      runData.realTime = workingRun.real_accumulated_time;
      runData.cpuTime = workingRun.cpu_accumulated_time;
      runData.bytesSecond = workingRun.bytes_per_second;
      runData.itemsSecond = workingRun.items_per_second;
      runData.range_x = workingRun.arg1;

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

      if(stddevState == 1) {
        stddevState++;
        workingRun = stddevRun;
      }
    } while(stddevState);
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
  const std::array<ChartIt, 4> lineCharts = {
    ChartIt{
     [](const RunData &mean) { return std::to_string(mean.cpuTime); },
     generateErrorbarCallable(&RunData::cpuTime)},
    ChartIt{
     [](const RunData &mean) { return std::to_string(mean.itemsSecond); },
     generateErrorbarCallable(&RunData::itemsSecond)},
    ChartIt{
     [](const RunData &mean) { return std::to_string(mean.realTime); },
     generateErrorbarCallable(&RunData::realTime)},
    ChartIt{
     [](const RunData &mean) { return std::to_string(mean.bytesSecond); },
     generateErrorbarCallable(&RunData::bytesSecond)}
   };
  std::string series;
  std::string chart(highChart_Line_Function);

  for(size_t chartNum = 0; chartNum < 4; chartNum++) {

    for (size_t n = 0; n < benchmarkTests_Line.size(); n++) {
      if(n > 0) {
        series.append(",");
      }

      series.append("{name: '").append(benchmarkTests_Line[n].name).append("',\ndata: [");
      for (size_t m = 0; m < benchmarkTests_Line[n].runData.size(); m++) {
        if(m > 0) {
            series.append(",");
        }
        series.append("[").append(std::to_string(benchmarkTests_Line[n].runData[m].range_x)).append(",");
        series.append(lineCharts[chartNum].value(benchmarkTests_Line[n].runData[m]));
        series.append("]");
      }
      series.append("]}");

        if(!benchmarkTests_Line_stddev.empty()) {
            series.append(",\n{type: 'errorbar',\nenableMouseTracking: false,\ndata: [");
            for(size_t m = 0; m < benchmarkTests_Line[n].runData.size(); m++) {
                if(m > 0) {
                    series.append(",");
                }
                series.append("[").append(std::to_string(benchmarkTests_Line[n].runData[m].range_x)).append(",").append(lineCharts[chartNum].error(benchmarkTests_Line[n].runData[m], benchmarkTests_Line_stddev[n].runData[m]));
                series.append("]");
            }
            series.append("]}\n");
        }
      }

      series.append("\n");

    replaceString(chart, "@YAXIS_TITLE@", yaxisTitles[chartNum]);
    replaceString(chart, "@TOOLTIP_BODY@", line_toolTipBodies[chartNum]);
    replaceString(chart, "@BENCHMARK_ID@", benchmarkId[chartNum]);
    replaceString(chart, "@BENCHMARK_NAME@",
                  benchmarkTitles[chartNum]);
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

void HTMLReporter::outputColumns(std::string &output) const {
  const char *benchmarkId[] = {
      "Benchmark_TimeInCpu_Column", "Benchmark_ItemsPerSec_Column",
      "Benchmark_TimeInReal_Column", "Benchmark_BytesPerSec_Column"};
  const std::array<ChartIt, 4> barCharts = {
    ChartIt{
     [](const RunData &mean) { return std::to_string(mean.cpuTime); },
     generateErrorbarCallable(&RunData::cpuTime)},
    ChartIt{
     [](const RunData &mean) { return std::to_string(mean.itemsSecond); },
     generateErrorbarCallable(&RunData::itemsSecond)},
    ChartIt{
     [](const RunData &mean) { return std::to_string(mean.realTime); },
     generateErrorbarCallable(&RunData::realTime)},
    ChartIt{
     [](const RunData &mean) { return std::to_string(mean.bytesSecond); },
     generateErrorbarCallable(&RunData::bytesSecond)}
   };
  std::string chart(highChart_Column_Function);
  std::string div(div_Element);
  std::string categories;
  std::string series;

  for(size_t chartNum = 0; chartNum < 4; chartNum++) {
    if(benchmarkTests_Column.size() > 0) {
        categories + "'" + benchmarkTests_Column[0].name + "'";

        for(size_t n = 1; n < benchmarkTests_Column.size(); n++) {
            categories + ", '" + benchmarkTests_Column[n].name + "'";
        }
    }

    series.append("{name: 'Benchmark',\ndata: [\n");
    for (size_t n = 0; n < benchmarkTests_Column.size(); n++) {
      if(n > 0) {
        series.append(",");
      }
      series.append("['").append(benchmarkTests_Column[n].name).append("',").append(barCharts[chartNum].value(benchmarkTests_Column[n].runData[0])).append("]");
    }

    series.append("]}");
        if(!benchmarkTests_Column_stddev.empty()) {
            series.append(",\n{type: 'errorbar',\nenableMouseTracking: false,\ndata: [");
            for(size_t n = 0; n < benchmarkTests_Column_stddev.size(); n++) {
                if(n > 0) {
                    series.append(",");
                }
                series.append("[").append(barCharts[chartNum].error(benchmarkTests_Column[n].runData[0], benchmarkTests_Column_stddev[n].runData[0])).append("]");
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
    replaceString(chart, "@BENCHMARK_NAME@",
                  benchmarkTitles[chartNum]);
    replaceString(chart, "@CATEGORIES@", categories);
    replaceString(chart, "@SERIES@", series);
    replaceString(output, "@CHART@", chart);
    replaceString(output, "@DIV@", div);

    categories.clear();
    series.clear();
    chart.assign(highChart_Column_Function);
    div.assign(div_Element);
  }
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
