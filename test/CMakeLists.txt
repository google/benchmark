# Demonstration executable
add_executable(benchmark_test benchmark_test.cc)
target_link_libraries(benchmark_test benchmark ${CMAKE_THREAD_LIBS_INIT})

# Test harness for regex wrapper
add_executable(re_test ${RE_FILES} "re_test.cc")
target_link_libraries(re_test benchmark_re gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
add_test(regex re_test)
