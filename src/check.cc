#include "check.h"

namespace benchmark {
namespace internal {

static AbortHandlerT* handler = &std::abort;

AbortHandlerT*& GetAbortHandler() { return handler; }

}  // namespace internal
}  // namespace benchmark
