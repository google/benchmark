cmake_minimum_required (VERSION 2.8)
project (benchmark)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "-Wall -Werror -pedantic-errors --std=c++0x")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-fno-strict-aliasing -O3 -DNDEBUG")

# Set OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_definitions(-DOS_MACOSX)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	add_definitions(-DOS_LINUX)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	add_definitions(-DOS_WINDOWS)
endif()

# Set CPU
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86")
	add_definitions(-DARCH_X86)
endif()

# Set up directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

# Build the targets
FILE(GLOB SOURCE_FILES "src/*.cc")
add_library(benchmark STATIC ${SOURCE_FILES})

add_executable(benchmark_test test/benchmark_test.cc)
target_link_libraries(benchmark_test benchmark ${CMAKE_THREAD_LIBS_INIT})

# Install target (will install the library to specified CMAKE_INSTALL_PREFIX variable)
INSTALL(
  TARGETS benchmark
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  COMPONENT library)