#include "check.h"

namespace benchmark::internal {

namespace {
AbortHandlerT* handler = &std::abort;
}  // namespace

BENCHMARK_EXPORT AbortHandlerT*& GetAbortHandler() { return handler; }

}  // namespace benchmark::internal
