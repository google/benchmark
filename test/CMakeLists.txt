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
add_test(benchmark benchmark_test --benchmark_min_time=0.1)

compile_benchmark_test(filter_test)
add_test(filter_simple filter_test --benchmark_filter=Calculate 16)
add_test(filter_suffix filter_test --benchmark_filter=Calculate* 16)
add_test(filter_regex_all filter_test --benchmark_filter=.* 16)
add_test(filter_regex_blank filter_test --benchmark_filter= 16)
add_test(filter_regex_none filter_test --benchmark_filter=monkey 0)
add_test(filter_regex_wildcard filter_test --benchmark_filter=.*Calculate.* 16)
add_test(filter_regex_begin filter_test --benchmark_filter=^BM_Calculate.* 16)
add_test(filter_regex_end filter_test --benchmark_filter=.*Pi$ 8)

compile_benchmark_test(basic_test)
add_test(basic basic_test)

compile_benchmark_test(cxx03_test)
set_target_properties(cxx03_test
    PROPERTIES COMPILE_FLAGS "${CXX03_FLAGS}")
add_test(cxx03 cxx03_test)

compile_benchmark_test(walltime_test)
