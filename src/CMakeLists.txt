set(SOURCE_FILES "benchmark.cc" "colorprint.cc" "commandlineflags.cc" "sleep.cc" "sysinfo.cc" "walltime.cc")
set(RE_FILES "re.cc")

add_library(benchmark_re STATIC ${RE_FILES})
add_library(benchmark STATIC ${SOURCE_FILES} ${RE_FILES})

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
