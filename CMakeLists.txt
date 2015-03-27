cmake_minimum_required (VERSION 2.8)
project (benchmark)

option(BENCHMARK_ENABLE_SHARED  "Enable building a shared library." OFF)
option(BENCHMARK_ENABLE_TESTING "Enable testing of the benchmark library." ON)
# Make sure we can import out CMake functions
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Read the git tags to determine the project version
include(GetGitVersion)
get_git_version(GIT_VERSION)

# Tell the user what versions we are using
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" VERSION ${GIT_VERSION})
message("-- Version: ${VERSION}")

# The version of the libraries
set(GENERIC_LIB_VERSION ${VERSION})
string(SUBSTRING ${VERSION} 0 1 GENERIC_LIB_SOVERSION)

# Try and enable C++11. Don't use C++14 because it doesn't work in some
# configurations.
include(CheckCXXCompilerFlag)
include(AddCXXCompilerFlag)
include(CXXFeatureCheck)

check_cxx_compiler_flag(-std=c++11 HAVE_FLAG_CXX_11)
check_cxx_compiler_flag(-std=c++0x HAVE_FLAG_CXX_0X)
if (HAVE_FLAG_CXX_11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif (HAVE_FLAG_CXX_0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

# Turn compiler warnings up to 11
add_cxx_compiler_flag(-Wall)
add_cxx_compiler_flag(-Wextra)
add_cxx_compiler_flag(-Wshadow)
add_cxx_compiler_flag(-Werror)
add_cxx_compiler_flag(-pedantic-errors)
add_cxx_compiler_flag(-Wshorten-64-to-32)
add_cxx_compiler_flag(-Wfloat-equal)
add_cxx_compiler_flag(-Wzero-as-null-pointer-constant)
# Release flags
add_cxx_compiler_flag(-fno-strict-aliasing RELEASE)

add_cxx_compiler_flag(-Wthread-safety)
if (HAVE_WTHREAD_SAFETY)
  add_definitions(-DHAVE_WTHREAD_SAFETY)
  cxx_feature_check(THREAD_SAFETY_ATTRIBUTES)
endif()

# C++ feature checks
cxx_feature_check(STD_REGEX)
cxx_feature_check(GNU_POSIX_REGEX)
cxx_feature_check(POSIX_REGEX)
cxx_feature_check(STEADY_CLOCK)

# Set up directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Build the targets
add_subdirectory(src)

if (BENCHMARK_ENABLE_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
