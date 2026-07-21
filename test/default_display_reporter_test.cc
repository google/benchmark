// Regression test for https://github.com/google/benchmark/issues/2240:
// CreateDefaultDisplayReporter() cached its result in a function-local static
// even though each caller owns (and frees) the returned pointer, so the
// second call returned a dangling pointer. It must hand out a fresh reporter
// on every call.

#include <iostream>
#include <memory>

#include "benchmark/benchmark.h"

int main(int argc, char** argv) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);
  benchmark::Initialize(&argc, argv);

  const std::unique_ptr<benchmark::BenchmarkReporter> first =
      benchmark::CreateDefaultDisplayReporter();
  const std::unique_ptr<benchmark::BenchmarkReporter> second =
      benchmark::CreateDefaultDisplayReporter();

  if (first == nullptr || second == nullptr || first.get() == second.get()) {
    std::cerr << "CreateDefaultDisplayReporter must return a fresh, "
                 "caller-owned reporter on every call\n";
    return 1;
  }

  return 0;
}
