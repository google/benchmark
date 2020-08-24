# Download and unpack gflags at configure time
set(GFLAGS_PREFIX "${benchmark_BINARY_DIR}/third_party/gflags")
configure_file(${benchmark_SOURCE_DIR}/cmake/GFlags.cmake.in ${GFLAGS_PREFIX}/CMakeLists.txt @ONLY)

set(GFLAGS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/gflags" CACHE PATH "") # Mind the quotes
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
  -DALLOW_DOWNLOADING_GFLAGS=${BENCHMARK_DOWNLOAD_DEPENDENCIES} -DGFLAGS_PATH:PATH=${GFLAGS_PATH} .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${GFLAGS_PREFIX}
)

if(result)
  message(FATAL_ERROR "CMake step for gflags failed: ${result}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${GFLAGS_PREFIX}
)

if(result)
  message(FATAL_ERROR "Build step for gflags failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gflags_force_shared_crt ON CACHE BOOL "" FORCE)

include(${GFLAGS_PREFIX}/gflags-paths.cmake)

# Add gflags directly to our build. This defines
# the gflags target.
add_subdirectory(${GFLAGS_SOURCE_DIR}
                 ${GFLAGS_BINARY_DIR}
                 EXCLUDE_FROM_ALL)

set_target_properties(gflags PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:gflags,INTERFACE_INCLUDE_DIRECTORIES>)
