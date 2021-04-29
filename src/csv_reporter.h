#include "benchmark/benchmark.h"

#include <set>
#include <string>
#include <vector>

namespace benchmark {

class BENCHMARK_DEPRECATED_MSG(
    "The CSV Reporter will be removed in a future release") CSVReporter
    : public BenchmarkReporter {
 public:
  CSVReporter() : printed_header_(false) {}
  virtual bool ReportContext(const Context& context);
  virtual void ReportRuns(const std::vector<Run>& reports);

 private:
  void PrintRunData(const Run& report);

  bool printed_header_;
  std::set<std::string> user_counter_names_;
};

}  // namespace benchmark
