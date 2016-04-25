#include "benchmark_util.h"
#include "string_util.h"
#include <algorithm>

namespace benchmark {
bool IsZero(double n) {
    return std::abs(n) < std::numeric_limits<double>::epsilon();
}

std::string GenerateInstanceName(const std::string& name, int arg_count,
                                 int arg1, int arg2, double min_time,
                                 bool use_real_time, bool multithreaded,
                                 int threads) {
  std::string instanceName(name);

  // Add arguments to instance name
  if (arg_count >= 1) {
    AppendHumanReadable(arg1, &instanceName);
  }
  if (arg_count >= 2) {
    AppendHumanReadable(arg2, &instanceName);
  }
  if (!IsZero(min_time)) {
    instanceName += StringPrintF("/min_time:%0.3f", min_time);
  }
  if (use_real_time) {
    instanceName += "/real_time";
  }

  // Add the number of threads used to the name
  if (multithreaded) {
    instanceName += StringPrintF("/threads:%d", threads);
  }

  return instanceName;
}
}
