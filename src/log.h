#ifndef BENCHMARK_LOG_H_
#define BENCHMARK_LOG_H_

#include <ostream>

namespace benchmark {
namespace internal {

std::ostream& GetNullLogInstance();
std::ostream& GetErrorLogInstance();

inline std::ostream& GetLogInstanceForLevel(int level) {
#if defined(BENCHMARK_LOGGING_LEVEL)
  if (level <= BENCHMARK_LOGGING_LEVEL) {
    return GetErrorLogInstance();
  }
#else
  ((void)level); // Use level so no warning is issued.
#endif
  return GetNullLogInstance();
}

} // end namespace internal
} // end namespace benchmark

#define VLOG(x) (::benchmark::internal::GetLogInstanceForLevel(x) \
                 << "-- LOG(" << x << "): ")

#endif