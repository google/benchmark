include(ExternalProject)

macro(split_list listname)
  string(REPLACE ";" " " ${listname} "${${listname}}")
endmacro()

if (NOT BENCHMARK_BUILD_EXTERNAL_GTEST)
  find_package(GTest REQUIRED)
else()
  set(GTEST_FLAGS "")
  if (BENCHMARK_USE_LIBCXX)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      list(APPEND GTEST_FLAGS -stdlib=libc++)
    else()
      message(WARNING "Unsupported compiler (${CMAKE_CXX_COMPILER}) when using libc++")
    endif()
  endif()
  if (BENCHMARK_BUILD_32_BITS)
    list(APPEND GTEST_FLAGS -m32)
  endif()
  if (NOT "${CMAKE_CXX_FLAGS}" STREQUAL "")
    list(APPEND GTEST_FLAGS ${CMAKE_CXX_FLAGS})
  endif()

  split_list(GTEST_FLAGS)
  string(REPLACE "-Wzero-as-null-pointer-constant" "" GTEST_FLAGS "${GTEST_FLAGS}")
  ExternalProject_Add(googletest
      EXCLUDE_FROM_ALL ON
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG master
      PREFIX "${CMAKE_BINARY_DIR}/googletest"
      INSTALL_DIR "${CMAKE_BINARY_DIR}/googletest"
      CMAKE_CACHE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_CXX_FLAGS:STRING=${GTEST_FLAGS}
      )

  ExternalProject_Get_Property(googletest install_dir)
  add_library(gtest UNKNOWN IMPORTED)
  add_library(gtest_main UNKNOWN IMPORTED)
  set_target_properties(gtest PROPERTIES
    IMPORTED_LOCATION ${install_dir}/lib/libgtest.a
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${install_dir}/include

  )
  set_target_properties(gtest_main PROPERTIES
    IMPORTED_LOCATION ${install_dir}/lib/libgtest_main.a
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${install_dir}/include
  )
  add_dependencies(gtest googletest)
  add_dependencies(gtest_main googletest)
  set(GTEST_BOTH_LIBRARIES gtest gtest_main)
  set(GTEST_INCLUDE_DIRS ${install_dir}/include)
endif()
