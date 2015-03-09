#ifndef BENCHMARK_SYSINFO_H_
#define BENCHMARK_SYSINFO_H_

namespace benchmark {
double MyCPUUsage();
double ChildrenCPUUsage();
int NumCPUs();
double CyclesPerSecond();
}  // end namespace benchmark

#endif  // BENCHMARK_SYSINFO_H_
