include(FetchContent)

set(ABSL_ENABLE_INSTALL ON)
if (NOT (DEFINED ABSEIL_PATH))
  message("ABSEIL_PATH not provided. Downloading.")
  FetchContent_Declare(
    abseil
    GIT_REPOSITORY    https://github.com/abseil/abseil-cpp.git
    GIT_TAG           4a23151e7ee089f54f0575f0a6d499e3a3fb6728
  )
  FetchContent_GetProperties(abseil)
  if(NOT abseil_POPULATED)
      FetchContent_Populate(abseil)
      add_subdirectory(${abseil_SOURCE_DIR} ${abseil_BINARY_DIR})
      set(ABSEIL_PATH ${abseil_SOURCE_DIR})
  endif()
else()
    add_subdirectory(${ABSEIL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/abseil-build)
endif()

include_directories(${ABSEIL_PATH})
list(APPEND BENCHMARK_CXX_LIBRARIES absl::flags absl::flags_parse)
