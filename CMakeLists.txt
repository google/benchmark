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

# Set up the compiler flags
include(CheckCXXCompilerFlag)

# Enable the latest C++ standard possible
check_cxx_compiler_flag(--std=c++14 HAVE_FLAG_CXX_14)
check_cxx_compiler_flag(--std=c++11 HAVE_FLAG_CXX_11)
check_cxx_compiler_flag(--std=c++0x HAVE_FLAG_CXX_0X)
if (HAVE_FLAG_CXX_14)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14")
elseif (HAVE_FLAG_CXX_11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
elseif (HAVE_FLAG_CXX_0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x")
endif()

# Turn compiler warnings up to 11
check_cxx_compiler_flag(-Wall HAVE_FLAG_WALL)
if (HAVE_FLAG_WALL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()
check_cxx_compiler_flag(-Wshadow HAVE_FLAG_WSHADOW)
if (HAVE_FLAG_WSHADOW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
endif()
check_cxx_compiler_flag(-Werror HAVE_FLAG_WERROR)
if (HAVE_FLAG_WERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()
check_cxx_compiler_flag(-pedantic-errors HAVE_FLAG_PEDANTIC_ERRORS)
if (HAVE_FLAG_PEDANTIC_ERRORS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic-errors")
endif()

# Add a debug definition so we can make decisions in the compilation
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# We relax the aliasing in release
check_cxx_compiler_flag(-fno-strict-aliasing HAVE_FLAG_NO_STRICT_ALIASING)
if (HAVE_FLAG_NO_STRICT_ALIASING)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-strict-aliasing")
endif()

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
