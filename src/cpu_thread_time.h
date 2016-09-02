#ifndef BENCHMARK_CPU_THREAD_TIME_H
#define BENCHMARK_CPU_THREAD_TIME_H

namespace benchmark {
double ProcessCPUUsage();
double ChildrenCPUUsage();
double ThreadCPUUsage();
}  // end namespace benchmark

#endif  // BENCHMARK_CPU_THREAD_TIME_H
