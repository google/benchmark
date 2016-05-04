#ifndef BENCHMARK_MPI_H
#define BENCHMARK_MPI_H

#cmakedefine BENCHMARK_ENABLE_MPI

#ifdef BENCHMARK_ENABLE_MPI
#include <mpi.h>
#endif

namespace benchmark {
#ifndef BENCHMARK_ENABLE_MPI
inline int mpi_world_size() { return 1; }
inline int mpi_world_rank() { return 0; }
inline double mpi_world_max(double value) { return value; }
inline double mpi_world_sum(double value) { return value; }
inline void initialize_mpi(int, char **) {}
inline void finalize_mpi() {}
#else
inline int mpi_world_size() {
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  return size;
}

inline int mpi_world_rank() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  return rank;
}

inline MPI_Datatype mpi_registered_type(double) { return MPI_DOUBLE; }
inline MPI_Datatype mpi_registered_type(int64_t) { return MPI_INT64_T; }

template <class T> T mpi_world_op(T value, MPI_Op const &operation) {
  T result;
  int const error =
      MPI_Allreduce(&value, &result, 1, mpi_registered_type(value), operation,
                    MPI_COMM_WORLD);
  if (error != MPI_SUCCESS)
    throw std::runtime_error("Encountered an mpi error");
  return result;
}

template <class T> T mpi_world_max(T value) {
  return mpi_world_op(value, MPI_MAX);
}
template <class T> T mpi_world_sum(T value) {
  return mpi_world_op(value, MPI_SUM);
}
inline void initialize_mpi(int argc, char **argv) { MPI_Init(&argc, &argv); }
inline void finalize_mpi() { MPI_Finalize(); }
#endif
inline bool mpi_is_world_root() { return mpi_world_rank() == 0; }
}
#endif
