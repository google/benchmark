# Define the source files
set(SOURCE_FILES "benchmark.cc" "colorprint.cc" "commandlineflags.cc" "sleep.cc" "sysinfo.cc" "walltime.cc")
set(RE_FILES "re.cc")

# Build a regular expression library
add_library(benchmark_re ${RE_FILES})
set_target_properties(benchmark_re PROPERTIES
  VERSION ${GENERIC_LIB_VERSION}
  SOVERSION ${GENERIC_LIB_SOVERSION}
)

# Build the benchmark library
add_library(benchmark ${SOURCE_FILES} ${RE_FILES})
set_target_properties(benchmark PROPERTIES
  VERSION ${GENERIC_LIB_VERSION}
  SOVERSION ${GENERIC_LIB_SOVERSION}
)

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
