//===---------------------------------------------------------------------===//
// disable_json_test - Test that the JSON header can be disabled by defining
//  BENCHMARK_HAS_NO_JSON_HEADER.
//===---------------------------------------------------------------------===//
#define BENCHMARK_HAS_NO_JSON_HEADER
#include "benchmark/benchmark.h"

#ifdef BENCHMARK_JSON_H
#error json.h should not be included.
#endif

namespace benchmark {
struct json {
  json() = delete;
};  // attempt to cause a duplicate definition error.
}  // namespace benchmark

int main() {}
