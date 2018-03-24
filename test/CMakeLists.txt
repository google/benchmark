# Enable the tests

find_package(Threads REQUIRED)
include(CheckCXXCompilerFlag)

# NOTE: Some tests use `<cassert>` to perform the test. Therefore we must
# strip -DNDEBUG from the default CMake flags in DEBUG mode.
string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
if( NOT uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG" )
  add_definitions( -UNDEBUG )
  add_definitions(-DTEST_BENCHMARK_LIBRARY_HAS_NO_ASSERTIONS)
  # Also remove /D NDEBUG to avoid MSVC warnings about conflicting defines.
  foreach (flags_var_to_scrub
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_C_FLAGS_MINSIZEREL)
    string (REGEX REPLACE "(^| )[/-]D *NDEBUG($| )" " "
      "${flags_var_to_scrub}" "${${flags_var_to_scrub}}")
  endforeach()
endif()

# NOTE: These flags must be added after find_package(Threads REQUIRED) otherwise
# they will break the configuration check.
if (DEFINED BENCHMARK_CXX_LINKER_FLAGS)
  list(APPEND CMAKE_EXE_LINKER_FLAGS ${BENCHMARK_CXX_LINKER_FLAGS})
endif()

add_library(output_test_helper STATIC output_test_helper.cc output_test.h)
target_link_libraries(output_test_helper benchmark)

function(add_benchmark_test target)
  cmake_parse_arguments(ARG
      "EXCLUDE_FROM_TEST;OUTPUT_TEST;GTEST_TEST"
      "SOURCE;REQUIRES"
      "COMPILE_FLAGS;LINK_FLAGS;LIBRARIES;TEST_ARGS"
      ${ARGN})
  if (NOT BENCHMARK_ENABLE_GTEST_TESTS AND ARG_GTEST_TEST)
    return()
  endif()
  if (ARG_REQUIRES)
    if (NOT ${ARG_REQUIRES})
      return()
    endif()
  endif()

  # Create the test executable
  if (NOT ARG_SOURCE)
    set(ARG_SOURCE "${target}.cc")
  endif()
  add_executable(${target} ${ARG_SOURCE})
  set_target_properties(${target}
      PROPERTIES
        COMPILE_FLAGS "${ARG_COMPILE_FLAGS}"
        LINK_FLAGS "${ARG_LINK_FLAGS}")
  target_link_libraries(${target} benchmark ${ARG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

  # Handle specifics for each different type of test. ex. output tests, gtests, ect.
  if (ARG_OUTPUT_TEST)
    target_link_libraries(${target} output_test_helper ${BENCHMARK_CXX_LIBRARIES})
  elseif (ARG_GTEST_TEST)
    if (TARGET googletest)
      add_dependencies(${target} googletest)
    endif()
    if (GTEST_INCLUDE_DIRS)
      target_include_directories(${target} PRIVATE ${GTEST_INCLUDE_DIRS})
    endif()
    target_link_libraries(${target} ${GTEST_BOTH_LIBRARIES})
  endif()

  # Unless otherwise specified, supply default arguments to each benchmark test.
  if (NOT ARG_TEST_ARGS AND NOT ARG_GTEST_TEST)
    set(ARG_TEST_ARGS --benchmark_min_time=0.01)
  endif()

  if (NOT ARG_EXCLUDE_FROM_TEST)
    add_test(${target} ${target} ${ARG_TEST_ARGS})
  endif()
endfunction(add_benchmark_test)

function(add_benchmark_tests)
  cmake_parse_arguments(ARG "" "" "OPTIONS" ${ARGN})
  set(TEST_TARGETS ${ARG_UNPARSED_ARGUMENTS})
  foreach(target ${TEST_TARGETS})
    add_benchmark_test(${target}
        ${ARG_OPTIONS})
  endforeach()
endfunction()

###############################################################################
# Filter Tests
###############################################################################
add_benchmark_test(filter_test EXCLUDE_FROM_TEST)
macro(add_filter_test name filter expect)
  add_test(${name} filter_test --benchmark_min_time=0.01 --benchmark_filter=${filter} ${expect})
  add_test(${name}_list_only filter_test --benchmark_list_tests --benchmark_filter=${filter} ${expect})
endmacro(add_filter_test)
add_filter_test(filter_simple "Foo" 3)
add_filter_test(filter_suffix "BM_.*" 4)
add_filter_test(filter_regex_all ".*" 5)
add_filter_test(filter_regex_blank "" 5)
add_filter_test(filter_regex_none "monkey" 0)
add_filter_test(filter_regex_wildcard ".*Foo.*" 3)
add_filter_test(filter_regex_begin "^BM_.*" 4)
add_filter_test(filter_regex_begin2 "^N" 1)
add_filter_test(filter_regex_end ".*Ba$" 1)

###############################################################################
# Basic Benchmark Tests
###############################################################################
add_benchmark_tests(
    benchmark_test
    options_test
    diagnostics_test
    skip_with_error_test
    donotoptimize_test
    fixture_test
    register_benchmark_test
    map_test
    multiple_ranges_test
)
add_benchmark_test(basic_benchmark SOURCE basic_test.cc)
# Some of the issues with DoNotOptimize only occur when optimization is enabled.
# Others only occur when they are disabled. Test both.
check_cxx_compiler_flag(-O3 BENCHMARK_HAS_O3_FLAG)
add_benchmark_test(donotoptimize_test_O3
      REQUIRES BENCHMARK_HAS_O3_FLAG
      SOURCE donotoptimize_test.cc
      COMPILE_FLAGS "-O3")
check_cxx_compiler_flag(-O0 BENCHMARK_HAS_O0_FLAG)
add_benchmark_test(donotoptimize_test_O0
      REQUIRES BENCHMARK_HAS_O0_FLAG
      SOURCE donotoptimize_test.cc
      COMPILE_FLAGS "-O0")
add_benchmark_tests(
    reporter_output_test 
    templated_fixture_test 
    user_counters_test
  OPTIONS
    OUTPUT_TEST
)
add_benchmark_test(
    user_counters_tabular_test
    OUTPUT_TEST
    TEST_ARGS --benchmark_counters_tabular=true --benchmark_min_time=0.01
)

# libstdc++ provides different definitions within <map> between dialects. When
# LTO is enabled and -Werror is specified GCC diagnoses this ODR violation
# causing the test to fail to compile. To prevent this we explicitly disable
# the warning.
check_cxx_compiler_flag(-Wno-odr BENCHMARK_HAS_WNO_ODR)
if (BENCHMARK_ENABLE_LTO AND BENCHMARK_HAS_WNO_ODR)
  set(LINK_FLAGS "-Wno-odr")
endif()
check_cxx_compiler_flag(-std=c++03 BENCHMARK_HAS_CXX03_FLAG)
add_benchmark_test(cxx03_test
    REQUIRES BENCHMARK_HAS_CXX03_FLAG
    COMPILE_FLAGS "-std=c++03"
    LINK_FLAGS "${LINK_FLAGS}")

# Attempt to work around flaky test failures when running on Appveyor servers.
if (DEFINED ENV{APPVEYOR})
  set(COMPLEXITY_MIN_TIME "0.5")
else()
  set(COMPLEXITY_MIN_TIME "0.01")
endif()
add_benchmark_test(complexity_test OUTPUT_TEST
    TEST_ARGS --benchmark_min_time=${COMPLEXITY_MIN_TIME})


###############################################################################
# GTEST Unit Tests
###############################################################################
add_benchmark_tests(
    statistics_test
  OPTIONS
    GTEST_TEST
)

###############################################################################
# Assembly Unit Tests
###############################################################################
if (BENCHMARK_ENABLE_ASSEMBLY_TESTS)
  if (NOT LLVM_FILECHECK_EXE)
    message(FATAL_ERROR "LLVM FileCheck is required when including this file")
  endif()
  include(AssemblyTests.cmake)
  add_filecheck_test(donotoptimize_assembly_test)
  add_filecheck_test(state_assembly_test)
  add_filecheck_test(clobber_memory_assembly_test)
endif()

###############################################################################
# Code Coverage Configuration
###############################################################################

# Add the coverage command(s)
if(CMAKE_BUILD_TYPE)
  string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)
endif()
if (${CMAKE_BUILD_TYPE_LOWER} MATCHES "coverage")
  find_program(GCOV gcov)
  find_program(LCOV lcov)
  find_program(GENHTML genhtml)
  find_program(CTEST ctest)
  if (GCOV AND LCOV AND GENHTML AND CTEST AND HAVE_CXX_FLAG_COVERAGE)
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/lcov/index.html
      COMMAND ${LCOV} -q -z -d .
      COMMAND ${LCOV} -q --no-external -c -b "${CMAKE_SOURCE_DIR}" -d . -o before.lcov -i
      COMMAND ${CTEST} --force-new-ctest-process
      COMMAND ${LCOV} -q --no-external -c -b "${CMAKE_SOURCE_DIR}" -d . -o after.lcov
      COMMAND ${LCOV} -q -a before.lcov -a after.lcov --output-file final.lcov
      COMMAND ${LCOV} -q -r final.lcov "'${CMAKE_SOURCE_DIR}/test/*'" -o final.lcov
      COMMAND ${GENHTML} final.lcov -o lcov --demangle-cpp --sort -p "${CMAKE_BINARY_DIR}" -t benchmark
      DEPENDS filter_test benchmark_test options_test basic_test fixture_test cxx03_test complexity_test
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Running LCOV"
    )
    add_custom_target(coverage
      DEPENDS ${CMAKE_BINARY_DIR}/lcov/index.html
      COMMENT "LCOV report at lcov/index.html"
    )
    message(STATUS "Coverage command added")
  else()
    if (HAVE_CXX_FLAG_COVERAGE)
      set(CXX_FLAG_COVERAGE_MESSAGE supported)
    else()
      set(CXX_FLAG_COVERAGE_MESSAGE unavailable)
    endif()
    message(WARNING
      "Coverage not available:\n"
      "  gcov: ${GCOV}\n"
      "  lcov: ${LCOV}\n"
      "  genhtml: ${GENHTML}\n"
      "  ctest: ${CTEST}\n"
      "  --coverage flag: ${CXX_FLAG_COVERAGE_MESSAGE}")
  endif()
endif()
