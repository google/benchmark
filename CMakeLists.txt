cmake_minimum_required (VERSION 2.8)
project (benchmark)

option(BENCHMARK_ENABLE_SHARED  "Enable building a shared library." ON)
option(BENCHMARK_ENABLE_TESTING "Enable testing of the benchmark library." ON)
set(BENCHMARK_LOGGING_LEVEL "0" CACHE STRING "The logging level to use.")

# Make sure we can import out CMake functions
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (NOT BENCHMARK_LOGGING_LEVEL STREQUAL "0")
  add_definitions(-DBENCHMARK_LOGGING_LEVEL=${BENCHMARK_LOGGING_LEVEL})
endif()

# Enable the latest C++ standard possible
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++11 HAVE_FLAG_CXX_11)
check_cxx_compiler_flag(-std=c++0x HAVE_FLAG_CXX_0X)
if (HAVE_FLAG_CXX_11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif (HAVE_FLAG_CXX_0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

# Turn compiler warnings up to 11
include(AddCXXCompilerFlag)
add_cxx_compiler_flag(-Wall)
add_cxx_compiler_flag(-Wextra)
add_cxx_compiler_flag(-Wshadow)
add_cxx_compiler_flag(-Werror)
add_cxx_compiler_flag(-pedantic-errors)
add_cxx_compiler_flag(-Wthread-safety)
# TODO(ericwf): enable this for g++
#add_cxx_compiler_flag(-Wzero-as-null-pointer-constant)

# Release flags
add_cxx_compiler_flag(-fno-strict-aliasing RELEASE)

# Set up directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Build the targets
add_subdirectory(src)

if (BENCHMARK_ENABLE_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
