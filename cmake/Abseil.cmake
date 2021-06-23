
set(ABSL_ENABLE_INSTALL ON)
if (NOT (DEFINED ABSEIL_PATH))
  message("ABSEIL_PATH not provided. Downloading.")
  set(ABSEIL_PREFIX "${benchmark_BINARY_DIR}/third_party/abseil")
  configure_file(${benchmark_SOURCE_DIR}/cmake/Abseil.cmake.in ${ABSEIL_PREFIX}/CMakeLists.txt @ONLY)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result 
    WORKING_DIRECTORY ${ABSEIL_PREFIX})
  if(result)
    message(FATAL_ERROR "CMake step for abseil failed: ${result}")
  endif()

  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    WORKING_DIRECTORY ${ABSEIL_PREFIX} RESULT_VARIABLE result)
  if(result)
    message(FATAL_ERROR "Download step for abseil failed: ${result}")
  endif()

  set(ABSEIL_PATH ${ABSEIL_PREFIX}/abseil-src)

  execute_process(COMMAND ${PYTHON_EXECUTABLE} 
    ${ABSEIL_PATH}/absl/copts/generate_copts.py 
    RESULT_VARIABLE result)
  if(result)
    message(FATAL_ERROR "Config step for abseil failed: ${result}")
  endif()

  add_subdirectory(${ABSEIL_PATH} ${ABSEIL_PREFIX}/abseil-build)
else()
    add_subdirectory(${ABSEIL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/abseil-build)
endif()

include_directories(${ABSEIL_PATH})
list(APPEND BENCHMARK_CXX_LIBRARIES absl::flags absl::flags_parse)
