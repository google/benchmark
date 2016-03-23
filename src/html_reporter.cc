#include "benchmark/reporter.h"
#include "benchmark/filePath.h"

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "string_util.h"
#include "walltime.h"

enum HTML_Reporter_State
{
    none, label, noLabel
};

const char *HTML_Begin = "<!DOCTYPE html>\n"
                         "<html>\n"
                         "   <head>\n"
                         "       <meta charset = \"UTF-8\"/>\n"
						 "       <title>Benchmark</title>\n"
						 "       <style type = \"text/css\">\n"
						 "           #benchmark{\n"
						 "                height:400px;\n"
						 "           }\n"
						 "       </style>\n";

const char *highChart_Line =
						 "       <script>\n"
						 "            $(function () {\n"
						 "                $('#benchmark').highcharts({\n"
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
						 "                        headerFormat: '<b><center>{series.name}</center></b><br/>',\n"
						 "                        pointFormat: 'Amount of calculated values: <b>{point.x}</b><br/>Time per item: <b>{point.y}ns</b>',\n"
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
						 "                   series:[\n";

const char *highChart_Colum_Part1 =
						 "       <script>\n"
						 "            $(function () {\n"
						 "                $('#benchmark').highcharts({\n"
						 "                    chart: {\n"
						 "                        type: 'column',\n"
						 "                        zoomType: 'xy'\n"
						 "                    },\n"
						 "                    title: {\n"
						 "                        text: 'Benchmark'\n"
						 "                    },\n"
						 "                    legend: {\n"
						 "                         enabled: false\n"
						 "                    },\n"
						 "                    xAxis: {\n"
						 "                        categories:[\n";

const char *highChart_Colum_Part2 =
						 "                    ]},\n"
						 "                    yAxis: {\n"
						 "                        title: {\n"
						 "                            text: 'Items per Second (items/s)'\n"
						 "                        },\n  "
						 "                        allowDecimals: true\n"
						 "                    },\n"
						 "                    tooltip: {\n"
                         "                        headerFormat: '<b><center>{point.key}</center></b><br/>',\n"
                         "                        pointFormat: 'Itemm per time: <b>{point.y}item/s</b>',\n"
						 "                        borderWidth: 5\n"
						 "                    },\n"
						 "                    scrollbar: {\n"
						 "                        enabled: true\n"
						 "                    },\n"
						 "                   series:[\n";

const char *HTML_End =   "                   ]\n"
                         "                });\n"
						 "            });\n"
						 "       </script>\n"
                         "   </head>\n"
						 "   <body>\n"
						 "      <div id = \"benchmark\"></div>\n"
						 "   </body>\n"
						 "</html>";

namespace benchmark {

HTMLReporter::HTMLReporter(const std::string &nUserString) : userString(nUserString) {
}

bool HTMLReporter::ReportContext(const Context& context) {

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
  std::cout << HTML_Begin;
  std::cout << "<script type = \"text/javascript\">";
  writeFile(JQUERY_PATH);
  std::cout << "</script>\n<script type = \"text/javascript\">";
  writeFile(HIGHSTOCK_PATH);
  std::cout << "</script>\n";

  state = HTML_Reporter_State::none;
  return true;
}

void HTMLReporter::ReportRuns(std::vector<Run> const& reports) {
    size_t subStrPos;
    RunData runData;
    size_t n;
    std::string name;
    std::string tail;

    if(state == HTML_Reporter_State::none) {
        determineState(reports[0].report_label);
    }

    if(state == HTML_Reporter_State::label) {
        bool hasRealTimeTag = false;

        //Remove x: form label
        subStrPos = reports[0].report_label.rfind("x:");
        if(subStrPos == std::string::npos) {
            return;
        }
        tail = reports[0].report_label.substr((subStrPos + 2));

        //Check for realTime
        hasRealTimeTag = (reports[0].benchmark_name.rfind("/real_time") != std::string::npos);

        //Remove all behind /
        subStrPos = reports[0].benchmark_name.find("/");
        if(subStrPos == std::string::npos) {
            return;
        }
        name = reports[0].benchmark_name.substr(0, subStrPos);
        if(hasRealTimeTag) {
            name.append("/real_time");
        }
        runData.range_x     = stoi(tail);
    }

    else {
        name = reports[0].benchmark_name;
    }

    runData.iterations  = reports[0].iterations;
    runData.realTime    = reports[0].real_accumulated_time;
    runData.cpuTime     = reports[0].cpu_accumulated_time;
    runData.bytesSecond = reports[0].bytes_per_second;
    runData.itemsSecond = reports[0].items_per_second;

    for(n = 0; n < benchmarkTests.size(); n++) {
        if(benchmarkTests[n].name == name) {
            benchmarkTests[n].runData.push_back(runData);
            break;
        }
    }

    if(n >= benchmarkTests.size()) {
        BenchmarkData benchmarkData;

        benchmarkData.name = std::move(name);
        benchmarkData.runData.push_back(runData);
        benchmarkTests.push_back(benchmarkData);
    }
}

void HTMLReporter::Finalize() {
    size_t n, m;

    if(state == HTML_Reporter_State::label) {
        std::cout << highChart_Line;
    }

    else {
        std::cout << highChart_Colum_Part1;
        m = 0;
        while(m < benchmarkTests.size()) {
            std::cout << "'" << benchmarkTests[m].name << "'";
            if(++m < benchmarkTests.size()) {
                std::cout << ",";
            }
        }
        std::cout << highChart_Colum_Part2;
    }

    n = 0;
    if(state == HTML_Reporter_State::label) {
        while(n < benchmarkTests.size())
        {
            std::cout << "{name: '" << benchmarkTests[n].name << "',\ndata: [";
            m = 0;
            while(m < benchmarkTests[n].runData.size()) {
                std::cout << "[" << benchmarkTests[n].runData[m].range_x << "," << nanoSecondsPerItem(benchmarkTests[n].runData[m].itemsSecond) << "]";

                if(++m < benchmarkTests[n].runData.size()) {
                    std::cout << ",";
                }
            }
            std::cout << "]}";

            if(++n < benchmarkTests.size()) {
                std::cout << ",";
            }

            std::cout << "\n";
        }
    }

    else {
        std::cout << "{name: 'Benchmark',\ndata: [\n";
        while(n < benchmarkTests.size()) {
            std::cout << "['" << benchmarkTests[n].name << "'," << benchmarkTests[n].runData[0].itemsSecond << "]";
            if(++n < benchmarkTests.size()) {
                std::cout << ",\n";
            }
        }
        std::cout << "]}\n";
    }

    std::cout << HTML_End;
    std::cout << userString;
}

double HTMLReporter::nanoSecondsPerItem(double itemsPerSec) const {
  double dItemsPerSec = itemsPerSec / 1000000000.0;

  return (1.0 / dItemsPerSec);
}

void HTMLReporter::determineState(const std::string &label) {
    if(label.rfind("x:") == std::string::npos){
        state = HTML_Reporter_State::noLabel;
    }

    else {
        state = HTML_Reporter_State::label;
    }
}

void HTMLReporter::writeFile(const char *file) const {
    std::fstream fin;
    char buffer[256];

    fin.open(file);
    if(fin.is_open()) {
        do {
            fin.read(buffer, 256);
            std::cout.write(buffer, fin.gcount());
        } while(fin.gcount() > 0);

        fin.close();
    }
}

}// end namespace benchmark
