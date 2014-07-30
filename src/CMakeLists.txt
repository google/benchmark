# Define the source files
set(SOURCE_FILES "benchmark.cc" "colorprint.cc" "commandlineflags.cc" "sleep.cc" "sysinfo.cc" "walltime.cc")

# Determine the correct regular expression engine to use
if(HAVE_STD_REGEX)
  set(RE_FILES "re_std.cc")
elseif(HAVE_GNU_POSIX_REGEX)
  set(RE_FILES "re_posix.cc")
elseif(HAVE_POSIX_REGEX)
  set(RE_FILES "re_posix.cc")
else()
  message(FATAL_ERROR "Failed to determine the source files for the regular expression backend")
endif()

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
