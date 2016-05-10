#ifndef BENCHMARK_MPI_H
#define BENCHMARK_MPI_H

#ifdef BENCHMARK_ENABLE_MPI
#include <mpi.h>
#endif

namespace benchmark {
#ifndef BENCHMARK_ENABLE_MPI
inline void initialize_mpi(int, char **) {}
inline void finalize_mpi() {}
#else
inline void initialize_mpi(int argc, char **argv) { MPI_Init(&argc, &argv); }
inline void finalize_mpi() { MPI_Finalize(); }
#endif
}
#endif
