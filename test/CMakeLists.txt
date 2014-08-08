# Demonstration executable
add_executable(benchmark_test benchmark_test.cc)
target_link_libraries(benchmark_test benchmark ${CMAKE_THREAD_LIBS_INIT})
add_test(benchmark benchmark_test 50)
add_test(benchmark_filter_simple benchmark_test --benchmark_filter=Calculate 16)
add_test(benchmark_filter_prefix benchmark_test --benchmark_filter=*Calculate 0)
add_test(benchmark_filter_suffix benchmark_test --benchmark_filter=Calculate* 16)
add_test(benchmark_filter_both benchmark_test --benchmark_filter=*Calculate* 0)
add_test(benchmark_filter_regex_wildcard benchmark_test --benchmark_filter=.*Calculate.* 16)

# Test harness for regex wrapper
add_executable(re_test ${RE_FILES} "re_test.cc")
target_link_libraries(re_test benchmark_re gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
add_test(regex re_test)
