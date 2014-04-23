# Demonstration executable
add_executable(benchmark_test benchmark_test.cc)
target_link_libraries(benchmark_test benchmark ${CMAKE_THREAD_LIBS_INIT})
