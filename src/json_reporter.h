#include "benchmark/benchmark.h"

#include <vector>

namespace benchmark {

class JSONReporter : public BenchmarkReporter {
 public:
  JSONReporter() : first_report_(true) {}
  virtual bool ReportContext(const Context& context);
  virtual void ReportRuns(const std::vector<Run>& reports);
  virtual void Finalize();

 private:
  void PrintRunData(const Run& report);

  bool first_report_;
};

}  // namespace benchmark
