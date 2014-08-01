# - Adds a compiler FLAG if it is supported by the compiler
#
# This function checks that the supplied compiler FLAG is supported and then
# adds it to the corresponding compiler FLAGs
#
#  add_cxx_compiler_FLAG(<FLAG> [<VARIANT>])
#
# - Example
#
# include(AddCXXCompilerFlag)
# add_cxx_compiler_FLAG(-Wall)
# add_cxx_compiler_FLAG(-no-strict-aliasing RELEASE)
# Requires CMake 2.6+

if(__add_cxx_compiler_FLAG)
  return()
endif()
set(__add_cxx_compiler_FLAG INCLUDED)

include(CheckCXXCompilerFlag)

function(add_cxx_compiler_flag FLAG)
  if(ARGV1)
    set(VARIANT ${ARGV1})
    string(TOLOWER ${VARIANT} VARIANT)
    set(VARIANT " ${VARIANT}")
  endif()
  message("-- Check compiler${VARIANT} flag ${FLAG}")
  string(TOUPPER ${FLAG} SANITIZED_FLAG)
  string(REGEX REPLACE "[^A-Za-z_0-9]" "_" ${SANITIZED_FLAG} SANITIZED_FLAG)
  check_cxx_compiler_flag(${FLAG} ${SANITIZED_FLAG})
  if(${SANITIZED_FLAG})
    message("-- Check compiler${VARIANT} flag ${FLAG} -- works")
    string(REGEX REPLACE "[^A-Za-z_0-9]" "_" "${VARIANT}" VARIANT)
    string(TOUPPER "${VARIANT}" VARIANT)
    set(CMAKE_CXX_FLAGS${VARIANT} "${CMAKE_CXX_FLAGS}${VARIANT} ${FLAG}" PARENT_SCOPE)
  endif()
endfunction()

