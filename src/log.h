#ifndef BENCHMARK_LOG_H_
#define BENCHMARK_LOG_H_

#include <ostream>
#include "benchmark_mpi.h"

namespace benchmark {
namespace internal {

int GetLogLevel();
void SetLogLevel(int level);

std::ostream& GetNullLogInstance();
std::ostream& GetErrorLogInstance();

inline std::ostream& GetLogInstanceForLevel(int level) {
  if (level <= GetLogLevel() and mpi_is_world_root()) {
    return GetErrorLogInstance();
  }
  return GetNullLogInstance();
}

} // end namespace internal
} // end namespace benchmark

#define VLOG(x) (::benchmark::internal::GetLogInstanceForLevel(x) \
                 << "-- LOG(" << x << "): ")

#endif
