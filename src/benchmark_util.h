#ifndef BENCHMARK_UTIL_H
#define BENCHMARK_UTIL_H
#include <string>

namespace benchmark {
bool IsZero(double n);

std::string GenerateInstanceName(const std::string& name, int arg_count,
                                 int arg1, int arg2, double min_time,
                                 bool use_real_time, bool multithreaded,
                                 int threads);
}
#endif // BENCHMARK_UTIL_H
