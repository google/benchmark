#include <chrono>
#include <iostream>
#include <string>

#include "benchmark/benchmark.h"

namespace {

// Macro to create benchmark families with many arguments
#define CREATE_BENCHMARK_FAMILY(name) \
  void name(benchmark::State& state) { \
    for (auto _ : state) {} \
  } \
  BENCHMARK(name)->DenseRange(0, 999, 1);

// Create many benchmark families, each with 1000 instances (args 0-999)
CREATE_BENCHMARK_FAMILY(BM_Alpha)
CREATE_BENCHMARK_FAMILY(BM_Beta)
CREATE_BENCHMARK_FAMILY(BM_Gamma)
CREATE_BENCHMARK_FAMILY(BM_Delta)
CREATE_BENCHMARK_FAMILY(BM_Epsilon)
CREATE_BENCHMARK_FAMILY(BM_Zeta)
CREATE_BENCHMARK_FAMILY(BM_Eta)
CREATE_BENCHMARK_FAMILY(BM_Theta)
CREATE_BENCHMARK_FAMILY(BM_Iota)
CREATE_BENCHMARK_FAMILY(BM_Kappa)
CREATE_BENCHMARK_FAMILY(BM_Lambda)
CREATE_BENCHMARK_FAMILY(BM_Mu)
CREATE_BENCHMARK_FAMILY(BM_Nu)
CREATE_BENCHMARK_FAMILY(BM_Xi)
CREATE_BENCHMARK_FAMILY(BM_Omicron)

// The target benchmark we're looking for (also with 1000 instances)
CREATE_BENCHMARK_FAMILY(BM_TargetBenchmark)

class NullReporter : public benchmark::BenchmarkReporter {
 public:
  bool ReportContext(const Context&) override { return true; }
  void ReportRuns(const std::vector<Run>&) override {}
  void Finalize() override {}
};

}  // namespace

int main(int argc, char** argv) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  
  std::cout << "\n=== Filter Optimization Performance Test ===\n";
  std::cout << "Total families: 16 (15 non-matching + 1 target)\n";
  std::cout << "Total instances: 16000 (16 families × 1000 args each)\n\n";
  
  // Measure time to filter with literal string (optimization applies)
  NullReporter null_reporter;
  
  std::cout << "Testing literal filter \"TargetBenchmark\"...\n";
  int argc1 = 3;
  const char* argv1[] = {"test", "--benchmark_filter=TargetBenchmark", "--benchmark_list_tests"};
  benchmark::Initialize(&argc1, const_cast<char**>(argv1));
  auto start_literal = std::chrono::high_resolution_clock::now();
  size_t count_literal = benchmark::RunSpecifiedBenchmarks(&null_reporter);
  auto end_literal = std::chrono::high_resolution_clock::now();
  auto duration_literal = std::chrono::duration_cast<std::chrono::microseconds>(end_literal - start_literal);
  
  std::cout << "Testing regex filter \".*TargetBenchmark.*\"...\n";
  int argc2 = 3;
  const char* argv2[] = {"test", "--benchmark_filter=.*TargetBenchmark.*", "--benchmark_list_tests"};
  benchmark::Initialize(&argc2, const_cast<char**>(argv2));
  auto start_regex = std::chrono::high_resolution_clock::now();
  size_t count_regex = benchmark::RunSpecifiedBenchmarks(&null_reporter);
  auto end_regex = std::chrono::high_resolution_clock::now();
  auto duration_regex = std::chrono::duration_cast<std::chrono::microseconds>(end_regex - start_regex);
  
  // Verify both found exactly 1000 benchmarks (all instances of BM_TargetBenchmark)
  if (count_literal != 1000) {
    std::cerr << "ERROR: Literal filter expected 1000 matches, got " << count_literal << "\n";
    return -1;
  }
  if (count_regex != 1000) {
    std::cerr << "ERROR: Regex filter expected 1000 matches, got " << count_regex << "\n";
    return -1;
  }
  
  std::cout << "\n=== RESULTS ===\n";
  std::cout << "Literal filter \"TargetBenchmark\":        " << duration_literal.count() << " μs\n";
  std::cout << "Regex filter \".*TargetBenchmark.*\":      " << duration_regex.count() << " μs\n\n";
  
  if (duration_literal.count() > 0) {
    double speedup = static_cast<double>(duration_regex.count()) / duration_literal.count();
    std::cout << "Speedup with optimization: " << speedup << "x faster\n\n";
    
    // Verify optimization provides at least 5x speedup
    if (speedup < 5.0) {
      std::cerr << "ERROR: Expected at least 5x speedup, got " << speedup << "x\n";
      std::cerr << "Optimization may not be working correctly!\n";
      return -1;
    }
  } else {
    std::cerr << "ERROR: Literal filter completed too fast to measure accurately\n";
    return -1;
  }
  
  std::cout << "=== WHY THE DIFFERENCE? ===\n";
  std::cout << "Literal filter (\"TargetBenchmark\"):\n";
  std::cout << "  - Detects no regex metacharacters\n";
  std::cout << "  - Uses family->name_.find(\"TargetBenchmark\")\n";
  std::cout << "  - Skips 15 families immediately (15000 instances not generated)\n";
  std::cout << "  - Only processes BM_TargetBenchmark family (1000 instances generated)\n\n";
  
  std::cout << "Regex filter (\".*TargetBenchmark.*\"):\n";
  std::cout << "  - Detects metacharacters (. and *)\n";
  std::cout << "  - Must process ALL 16 families\n";
  std::cout << "  - Generates all 16000 instances and regex-matches each name\n\n";
    
  return 0;
}
