# Exports google benchmark so other packages can access it
export(TARGETS google.benchmark FILE "${PROJECT_BINARY_DIR}/BenchmarkTargets.cmake")

# Avoids creating an entry in the cmake registry.
if(NOT NOEXPORT)
    export(PACKAGE Benchmark)
endif()

configure_File(cmake/BenchmarkConfig.in.cmake
    "${PROJECT_BINARY_DIR}/BenchmarkConfig.cmake" @ONLY
)
configure_File(cmake/BenchmarkConfigVersion.in.cmake
    "${PROJECT_BINARY_DIR}/BenchmarkConfigVersion.cmake" @ONLY
)

install(FILES
    "${PROJECT_BINARY_DIR}/BenchmarkConfig.cmake"
    "${PROJECT_BINARY_DIR}/BenchmarkConfigVersion.cmake"
    DESTINATION share/cmake/benchmark
    COMPONENT dev
)

install(EXPORT BenchmarkTargets DESTINATION share/cmake/Benchmark)
