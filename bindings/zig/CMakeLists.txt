# CMake build for the Zig bindings C++ adapter.
#
# This CMakeLists.txt builds both libbenchmark and the C adapter (zig_api.cc)
# using the same compiler (g++), then merges them into a single combined
# archive. This avoids C++ ABI mismatches between libstdc++ and libc++.
#
# The combined archive (libbenchmark_combined.a) is what the Zig build
# links against — it contains all C++ symbols in one place.
#
# Usage:
#   cmake -S bindings/zig -B bindings/zig/cmake-build -DCMAKE_BUILD_TYPE=Release
#   cmake --build bindings/zig/cmake-build --parallel

cmake_minimum_required(VERSION 3.13)
project(google_benchmark_zig CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build libbenchmark from the project root (../../)
add_subdirectory(../../ benchmark_build EXCLUDE_FROM_ALL)

# Build the C adapter library.
# Compiled with the same compiler as libbenchmark to ensure ABI compatibility.
add_library(zig_api STATIC src/zig_api.cc)
target_include_directories(zig_api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
target_compile_options(zig_api PRIVATE -fno-exceptions)

# Create a combined archive containing both libbenchmark and zig_api objects.
# This ensures ALL C++ code uses the same ABI (no libstdc++/libc++ conflicts).
set(COMBINED_LIB ${CMAKE_CURRENT_BINARY_DIR}/libbenchmark_combined.a)
set(TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/ar_tmp)

add_custom_target(combine_libs ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${TMP_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TMP_DIR}
    COMMAND ${CMAKE_COMMAND} -E chdir ${TMP_DIR} ${CMAKE_AR} x $<TARGET_FILE:benchmark>
    COMMAND ${CMAKE_COMMAND} -E chdir ${TMP_DIR} ${CMAKE_AR} x $<TARGET_FILE:zig_api>
    COMMAND ${CMAKE_AR} rcs ${COMBINED_LIB} ${TMP_DIR}/*.o
    COMMAND ${CMAKE_RANLIB} ${COMBINED_LIB}
    DEPENDS benchmark zig_api
    COMMENT "Creating combined benchmark+adapter archive"
)
