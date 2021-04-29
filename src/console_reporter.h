#include "benchmark/benchmark.h"

#include <vector>

namespace benchmark {

// Simple reporter that outputs benchmark data to the console. This is the
// default reporter used by RunSpecifiedBenchmarks().
class ConsoleReporter : public BenchmarkReporter {
 public:
  enum OutputOptions {
    OO_None = 0,
    OO_Color = 1,
    OO_Tabular = 2,
    OO_ColorTabular = OO_Color | OO_Tabular,
    OO_Defaults = OO_ColorTabular
  };
  explicit ConsoleReporter(OutputOptions opts_ = OO_Defaults)
      : output_options_(opts_),
        name_field_width_(0),
        prev_counters_(),
        printed_header_(false) {}

  virtual bool ReportContext(const Context& context);
  virtual void ReportRuns(const std::vector<Run>& reports);

 protected:
  virtual void PrintRunData(const Run& report);
  virtual void PrintHeader(const Run& report);

  OutputOptions output_options_;
  size_t name_field_width_;
  UserCounters prev_counters_;
  bool printed_header_;
};

}  // namespace benchmark
