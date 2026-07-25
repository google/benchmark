cmake_minimum_required(VERSION 3.13)
project(google_benchmark_zig CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build libbenchmark from source
add_subdirectory(../../ benchmark_build EXCLUDE_FROM_ALL)

# Build the C++ adapter (compiled with same g++ as libbenchmark)
add_library(zig_api STATIC src/zig_api.cc)
target_include_directories(zig_api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
target_compile_options(zig_api PRIVATE -fno-exceptions)

# Create a combined archive by merging both archives
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
