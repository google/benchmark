# Enable the tests

# Import and build Google Test
include(ExternalProject)
set_directory_properties(properties EP_PREFIX "${CMAKE_BINARY_DIR}/third_party")
ExternalProject_Add(googletest
  URL "https://googletest.googlecode.com/files/gtest-1.7.0.zip"
  URL_MD5 2d6ec8ccdf5c46b05ba54a9fd1d130d7
  SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/gtest"
  CMAKE_ARGS "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
  INSTALL_COMMAND "")

ExternalProject_Get_Property(googletest source_dir)
ExternalProject_Get_Property(googletest binary_dir)
include_directories(${source_dir}/include)
link_directories(${binary_dir})

find_package(Threads REQUIRED)

macro(add_gtest_test name)
  add_executable(${name} "${name}.cc")
  target_link_libraries(${name} benchmark gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
  add_dependencies(${name} googletest)
  add_test(${name} ${name})
endmacro(add_gtest_test)

macro (add_benchmark_test name)
  add_executable(${name} "${name}.cc")
  target_link_libraries(${name} benchmark gtest ${CMAKE_THREAD_LIBS_INIT})
  add_dependencies(${name} googletest)
  add_test(${name} ${name} --benchmarks=all)
endmacro(add_benchmark_test)

# Demonstration executable
add_benchmark_test(benchmark_test)
add_benchmark_test(basic_test)
add_gtest_test(re_test)