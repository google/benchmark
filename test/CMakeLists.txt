# Enable the tests

find_package(Threads REQUIRED)

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
