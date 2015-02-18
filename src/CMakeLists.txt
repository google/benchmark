# Allow the source files to find headers in src/
include_directories(${PROJECT_SOURCE_DIR}/src)

# Define the source files
set(SOURCE_FILES "benchmark.cc" "minimal_benchmark.cc" "commandlineflags.cc"
                 "log.cc" "sleep.cc" "string_util.cc" "sysinfo.cc"
                 "walltime.cc")

# Build the benchmark library
if (BENCHMARK_ENABLE_SHARED)
  add_library(benchmark SHARED ${SOURCE_FILES})
  find_package(Threads REQUIRED)
  target_link_libraries(benchmark ${CMAKE_THREAD_LIBS_INIT})
else()
  add_library(benchmark STATIC ${SOURCE_FILES})
endif()

set_target_properties(benchmark PROPERTIES
  OUTPUT_NAME "benchmark"
  VERSION "2.0"
  SOVERSION "2")

# Install target (will install the library to specified CMAKE_INSTALL_PREFIX variable)
install(
  TARGETS benchmark
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  COMPONENT library)

install(
  DIRECTORY "${PROJECT_SOURCE_DIR}/include/benchmark"
  DESTINATION include
  FILES_MATCHING PATTERN "*.*h")
