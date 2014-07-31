# - Compile and run code to check for C++ features
#
# This functions compiles a source file under the `cmake` folder
# and adds the corresponding `HAVE_[FILENAME]` flag to the CMake
# environment
#
#  add_cxx_compiler_FLAG(<FLAG> [<VARIANT>])
#
# - Example
#
# include(AddCXXCompilerFlag)
# add_cxx_compiler_FLAG(-Wall)
# add_cxx_compiler_FLAG(-no-strict-aliasing RELEASE)
# Requires CMake 2.6+

if(__cxx_feature_check_FLAG)
  return()
endif()
set(__cxx_feature_check_FLAG INCLUDED)

function(cxx_feature_check FILE)
  string(TOLOWER ${FILE} FILE)
  string(TOUPPER ${FILE} VAR)
  string(TOUPPER "HAVE_${VAR}" FEATURE)
  message("-- Performing Test ${FEATURE}")
  try_run(RUN_${FEATURE} COMPILE_${FEATURE} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${FILE}.cpp)
  if(RUN_${FEATURE} EQUAL 0)
    message("-- Performing Test ${FEATURE} -- Success")
    set(HAVE_${VAR} 1 PARENT_SCOPE)
    add_definitions(-DHAVE_${VAR})
  else()
    message("-- Performing Test ${FEATURE} -- Failed")
  endif()
endfunction()

