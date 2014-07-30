cmake_minimum_required (VERSION 2.8)
project (benchmark)

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
execute_process(COMMAND git describe --match "v[0-9]*.[0-9]*.[0-9]*" --abbrev=8
    RESULT_VARIABLE status
    OUTPUT_VARIABLE GIT_VERSION
    ERROR_QUIET)
if(${status})
    set(GIT_VERSION "v0.0.0")
else()
    string(STRIP ${GIT_VERSION} GIT_VERSION)
    string(REGEX REPLACE "-[0-9]+-g" "-" GIT_VERSION ${GIT_VERSION})
endif()

# Work out if the repository is dirty
execute_process(COMMAND git update-index -q --refresh
    OUTPUT_QUIET
    ERROR_QUIET)
execute_process(COMMAND git diff-index --name-only HEAD --
    OUTPUT_VARIABLE GIT_DIFF_INDEX
    ERROR_QUIET)
string(COMPARE NOTEQUAL "${GIT_DIFF_INDEX}" "" GIT_DIRTY)
if (${GIT_DIRTY})
    string(CONCAT GIT_VERSION ${GIT_VERSION} "-dirty")
endif()

# Tell the user what versions we are using
message("-- git Version: ${GIT_VERSION}")
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
