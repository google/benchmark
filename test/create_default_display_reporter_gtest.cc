#include <memory>

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace benchmark {
namespace {

// Regression test for a heap-use-after-free (issue #2240):
// CreateDefaultDisplayReporter() transfers ownership of the returned reporter
// to the caller (RunSpecifiedBenchmarks() wraps it in a unique_ptr and deletes
// it). It must therefore return a fresh object on every call. A previous
// implementation cached the pointer in a function-local `static`, so the second
// call returned an already-deleted reporter -> use-after-free.
TEST(CreateDefaultDisplayReporterTest, ReturnsFreshOwnedReporterEachCall) {
  std::unique_ptr<BenchmarkReporter> first(CreateDefaultDisplayReporter());
  std::unique_ptr<BenchmarkReporter> second(CreateDefaultDisplayReporter());

  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  // Distinct objects: neither call hands back a shared/cached pointer that the
  // other unique_ptr would also try to delete (double-free / use-after-free).
  EXPECT_NE(first.get(), second.get());
}

}  // namespace
}  // namespace benchmark
