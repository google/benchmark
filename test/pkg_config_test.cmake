include("${PROJECT_SOURCE_DIR}/cmake/GoogleBenchmarkPkgConfig.cmake")

function(expect_pkgconfig_path input expected)
  benchmark_make_pkgconfig_path(actual "${input}")
  if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "Expected '${expected}' for '${input}', got '${actual}'")
  endif()
endfunction()

expect_pkgconfig_path("lib" "\${prefix}/lib")
expect_pkgconfig_path("include/benchmark" "\${prefix}/include/benchmark")
expect_pkgconfig_path("/nix/store/google-benchmark/lib" "/nix/store/google-benchmark/lib")
expect_pkgconfig_path("/nix/store/google-benchmark/include" "/nix/store/google-benchmark/include")
