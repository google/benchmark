# Enable the tests

find_package(Threads REQUIRED)

set(CXX03_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-std=c++11" "-std=c++03" CXX03_FLAGS "${CXX03_FLAGS}")
string(REPLACE "-std=c++0x" "-std=c++03" CXX03_FLAGS "${CXX03_FLAGS}")

macro(compile_benchmark_test name)
  add_executable(${name} "${name}.cc")
  target_link_libraries(${name} benchmark ${CMAKE_THREAD_LIBS_INIT})
endmacro(compile_benchmark_test)

# Demonstration executable
compile_benchmark_test(benchmark_test)
add_test(benchmark benchmark_test --benchmark_min_time=0.01)

compile_benchmark_test(filter_test)
macro(add_filter_test name filter expect)
  add_test(${name} filter_test --benchmark_min_time=0.01 --benchmark_filter=${filter} ${expect})
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

compile_benchmark_test(options_test)
add_test(options_benchmarks options_test --benchmark_min_time=0.01)

compile_benchmark_test(basic_test)
add_test(basic_benchmark basic_test --benchmark_min_time=0.01)

compile_benchmark_test(cxx03_test)
set_target_properties(cxx03_test
    PROPERTIES COMPILE_FLAGS "${CXX03_FLAGS}")
add_test(cxx03 cxx03_test --benchmark_min_time=0.01)
