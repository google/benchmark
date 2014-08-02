cmake_minimum_required (VERSION 2.8)
project (benchmark)

# Make sure we can import out CMake functions
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# We need threads in this project
find_package(Threads REQUIRED)

# Import and build Google Test
include(ExternalProject)
set_directory_properties(properties EP_PREFIX "${CMAKE_BINARY_DIR}/third_party")
ExternalProject_Add(googletest
    URL "https://googletest.googlecode.com/files/gtest-1.7.0.zip"
    SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/gtest"
    CMAKE_ARGS "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    INSTALL_COMMAND "")
ExternalProject_Get_Property(googletest source_dir)
include_directories(${source_dir}/include)
ExternalProject_Get_Property(googletest binary_dir)
link_directories(${binary_dir})

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

# Read the git tags to determine the project version
include(GetGitVersion)
get_git_version(GIT_VERSION)

# Tell the user what versions we are using
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" VERSION ${GIT_VERSION})
message("-- Version: ${VERSION}")

# The version of the libraries
set(GENERIC_LIB_VERSION ${VERSION})
string(SUBSTRING ${VERSION} 0 1 GENERIC_LIB_SOVERSION)

# Set up directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

# Build the targets
enable_testing()
add_subdirectory(src)
add_subdirectory(test)
