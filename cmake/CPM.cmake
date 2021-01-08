# CPM.cmake - CMake's missing package manager
# ===========================================
# See https://github.com/TheLartians/CPM.cmake for usage and update instructions.
#
# MIT License
# -----------
#[[
  Copyright (c) 2019 Lars Melchior

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
]]

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(CURRENT_CPM_VERSION 0.27.5)

if(CPM_DIRECTORY)
  if(NOT CPM_DIRECTORY STREQUAL CMAKE_CURRENT_LIST_DIR)
    if (CPM_VERSION VERSION_LESS CURRENT_CPM_VERSION)
      message(AUTHOR_WARNING "${CPM_INDENT} \
A dependency is using a more recent CPM version (${CURRENT_CPM_VERSION}) than the current project (${CPM_VERSION}). \
It is recommended to upgrade CPM to the most recent version. \
See https://github.com/TheLartians/CPM.cmake for more information."
      )
    endif()
    return()
  endif()

  get_property(CPM_INITIALIZED GLOBAL "" PROPERTY CPM_INITIALIZED SET)
  if (CPM_INITIALIZED)
    return()
  endif()
endif()

set_property(GLOBAL PROPERTY CPM_INITIALIZED true)

option(CPM_USE_LOCAL_PACKAGES "Always try to use `find_package` to get dependencies" $ENV{CPM_USE_LOCAL_PACKAGES})
option(CPM_LOCAL_PACKAGES_ONLY "Only use `find_package` to get dependencies" $ENV{CPM_LOCAL_PACKAGES_ONLY})
option(CPM_DOWNLOAD_ALL "Always download dependencies from source" $ENV{CPM_DOWNLOAD_ALL})
option(CPM_DONT_UPDATE_MODULE_PATH "Don't update the module path to allow using find_package" $ENV{CPM_DONT_UPDATE_MODULE_PATH})
option(CPM_DONT_CREATE_PACKAGE_LOCK "Don't create a package lock file in the binary path" $ENV{CPM_DONT_CREATE_PACKAGE_LOCK})
option(CPM_INCLUDE_ALL_IN_PACKAGE_LOCK "Add all packages added through CPM.cmake to the package lock" $ENV{CPM_INCLUDE_ALL_IN_PACKAGE_LOCK})

set(CPM_VERSION ${CURRENT_CPM_VERSION} CACHE INTERNAL "")
set(CPM_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")
set(CPM_FILE ${CMAKE_CURRENT_LIST_FILE} CACHE INTERNAL "")
set(CPM_PACKAGES "" CACHE INTERNAL "")
set(CPM_DRY_RUN OFF CACHE INTERNAL "Don't download or configure dependencies (for testing)")

if(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_SOURCE_CACHE_DEFAULT $ENV{CPM_SOURCE_CACHE})
else()
  set(CPM_SOURCE_CACHE_DEFAULT OFF)
endif()

set(CPM_SOURCE_CACHE ${CPM_SOURCE_CACHE_DEFAULT} CACHE PATH "Directory to downlaod CPM dependencies")

if (NOT CPM_DONT_UPDATE_MODULE_PATH)
  set(CPM_MODULE_PATH "${CMAKE_BINARY_DIR}/CPM_modules" CACHE INTERNAL "")
  # remove old modules
  FILE(REMOVE_RECURSE ${CPM_MODULE_PATH})
  file(MAKE_DIRECTORY ${CPM_MODULE_PATH})
  # locally added CPM modules should override global packages
  set(CMAKE_MODULE_PATH "${CPM_MODULE_PATH};${CMAKE_MODULE_PATH}")
endif()

if (NOT CPM_DONT_CREATE_PACKAGE_LOCK)
  set(CPM_PACKAGE_LOCK_FILE "${CMAKE_BINARY_DIR}/cpm-package-lock.cmake" CACHE INTERNAL "")
  file(WRITE ${CPM_PACKAGE_LOCK_FILE} "# CPM Package Lock\n# This file should be committed to version control\n\n")
endif()

include(FetchContent)
include(CMakeParseArguments)

# Initialize logging prefix
if(NOT CPM_INDENT)
  set(CPM_INDENT "CPM:")
endif()

function(cpm_find_package NAME VERSION)
  string(REPLACE " " ";" EXTRA_ARGS "${ARGN}")
  find_package(${NAME} ${VERSION} ${EXTRA_ARGS} QUIET)
  if(${CPM_ARGS_NAME}_FOUND)
    message(STATUS "${CPM_INDENT} using local package ${CPM_ARGS_NAME}@${VERSION}")
    CPMRegisterPackage(${CPM_ARGS_NAME} "${VERSION}")
    set(CPM_PACKAGE_FOUND YES PARENT_SCOPE)
  else()
    set(CPM_PACKAGE_FOUND NO PARENT_SCOPE)
  endif()
endfunction()

# Create a custom FindXXX.cmake module for a CPM package
# This prevents `find_package(NAME)` from finding the system library
function(CPMCreateModuleFile Name)
  if (NOT CPM_DONT_UPDATE_MODULE_PATH)
    # erase any previous modules
    FILE(WRITE ${CPM_MODULE_PATH}/Find${Name}.cmake "include(${CPM_FILE})\n${ARGN}\nset(${Name}_FOUND TRUE)")
  endif()
endfunction()

# Find a package locally or fallback to CPMAddPackage
function(CPMFindPackage)
  set(oneValueArgs
    NAME
    VERSION
    GIT_TAG
    FIND_PACKAGE_ARGUMENTS
  )

  cmake_parse_arguments(CPM_ARGS "" "${oneValueArgs}" "" ${ARGN})

  if (NOT DEFINED CPM_ARGS_VERSION)
    if (DEFINED CPM_ARGS_GIT_TAG)
      cpm_get_version_from_git_tag("${CPM_ARGS_GIT_TAG}" CPM_ARGS_VERSION)
    endif()
  endif()

  if (CPM_DOWNLOAD_ALL)
    CPMAddPackage(${ARGN})
    cpm_export_variables(${CPM_ARGS_NAME})
    return()
  endif()

  CPMCheckIfPackageAlreadyAdded(${CPM_ARGS_NAME} "${CPM_ARGS_VERSION}" "${CPM_ARGS_OPTIONS}")
  if (CPM_PACKAGE_ALREADY_ADDED)
    cpm_export_variables(${CPM_ARGS_NAME})
    return()
  endif()

  cpm_find_package(${CPM_ARGS_NAME} "${CPM_ARGS_VERSION}" ${CPM_ARGS_FIND_PACKAGE_ARGUMENTS})

  if(NOT CPM_PACKAGE_FOUND)
    CPMAddPackage(${ARGN})
    cpm_export_variables(${CPM_ARGS_NAME})
  endif()

endfunction()

# checks if a package has been added before
function(CPMCheckIfPackageAlreadyAdded CPM_ARGS_NAME CPM_ARGS_VERSION CPM_ARGS_OPTIONS)
  if ("${CPM_ARGS_NAME}" IN_LIST CPM_PACKAGES)
    CPMGetPackageVersion(${CPM_ARGS_NAME} CPM_PACKAGE_VERSION)
    if("${CPM_PACKAGE_VERSION}" VERSION_LESS "${CPM_ARGS_VERSION}")
      message(WARNING "${CPM_INDENT} requires a newer version of ${CPM_ARGS_NAME} (${CPM_ARGS_VERSION}) than currently included (${CPM_PACKAGE_VERSION}).")
    endif()
    if (CPM_ARGS_OPTIONS)
      foreach(OPTION ${CPM_ARGS_OPTIONS})
        cpm_parse_option(${OPTION})
        if(NOT "${${OPTION_KEY}}" STREQUAL "${OPTION_VALUE}")
          message(WARNING "${CPM_INDENT} ignoring package option for ${CPM_ARGS_NAME}: ${OPTION_KEY} = ${OPTION_VALUE} (${${OPTION_KEY}})")
        endif()
      endforeach()
    endif()
    cpm_get_fetch_properties(${CPM_ARGS_NAME})
    SET(${CPM_ARGS_NAME}_ADDED NO)
    SET(CPM_PACKAGE_ALREADY_ADDED YES PARENT_SCOPE)
    cpm_export_variables(${CPM_ARGS_NAME})
  else()
    SET(CPM_PACKAGE_ALREADY_ADDED NO PARENT_SCOPE)
  endif()
endfunction()

# Download and add a package from source
function(CPMAddPackage)

  set(oneValueArgs
    NAME
    FORCE
    VERSION
    GIT_TAG
    DOWNLOAD_ONLY
    GITHUB_REPOSITORY
    GITLAB_REPOSITORY
    GIT_REPOSITORY
    SOURCE_DIR
    DOWNLOAD_COMMAND
    FIND_PACKAGE_ARGUMENTS
    NO_CACHE
    GIT_SHALLOW
  )

  set(multiValueArgs
    OPTIONS
  )

  cmake_parse_arguments(CPM_ARGS "" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

  # Set default values for arguments

  if (NOT DEFINED CPM_ARGS_VERSION)
    if (DEFINED CPM_ARGS_GIT_TAG)
      cpm_get_version_from_git_tag("${CPM_ARGS_GIT_TAG}" CPM_ARGS_VERSION)
    endif()
  endif()

  if(CPM_ARGS_DOWNLOAD_ONLY)
    set(DOWNLOAD_ONLY ${CPM_ARGS_DOWNLOAD_ONLY})
  else()
    set(DOWNLOAD_ONLY NO)
  endif()

  if (DEFINED CPM_ARGS_GITHUB_REPOSITORY)
    set(CPM_ARGS_GIT_REPOSITORY "https://github.com/${CPM_ARGS_GITHUB_REPOSITORY}.git")
  endif()

  if (DEFINED CPM_ARGS_GITLAB_REPOSITORY)
    set(CPM_ARGS_GIT_REPOSITORY "https://gitlab.com/${CPM_ARGS_GITLAB_REPOSITORY}.git")
  endif()

  if (DEFINED CPM_ARGS_GIT_REPOSITORY)
    list(APPEND CPM_ARGS_UNPARSED_ARGUMENTS GIT_REPOSITORY ${CPM_ARGS_GIT_REPOSITORY})
    if (NOT DEFINED CPM_ARGS_GIT_TAG)
      set(CPM_ARGS_GIT_TAG v${CPM_ARGS_VERSION})
    endif()
  endif()

  if (DEFINED CPM_ARGS_GIT_TAG)
    list(APPEND CPM_ARGS_UNPARSED_ARGUMENTS GIT_TAG ${CPM_ARGS_GIT_TAG})
    # If GIT_SHALLOW is explicitly specified, honor the value.
    if (DEFINED CPM_ARGS_GIT_SHALLOW)
      list(APPEND CPM_ARGS_UNPARSED_ARGUMENTS GIT_SHALLOW ${CPM_ARGS_GIT_SHALLOW})
    endif()
  endif()

  # Check if package has been added before
  CPMCheckIfPackageAlreadyAdded(${CPM_ARGS_NAME} "${CPM_ARGS_VERSION}" "${CPM_ARGS_OPTIONS}")
  if (CPM_PACKAGE_ALREADY_ADDED)
    cpm_export_variables(${CPM_ARGS_NAME})
    return()
  endif()

  # Check for manual overrides
  if (NOT CPM_ARGS_FORCE AND NOT "${CPM_${CPM_ARGS_NAME}_SOURCE}" STREQUAL "")
    set(PACKAGE_SOURCE ${CPM_${CPM_ARGS_NAME}_SOURCE})
    set(CPM_${CPM_ARGS_NAME}_SOURCE "")
    CPMAddPackage(
      NAME ${CPM_ARGS_NAME}
      SOURCE_DIR ${PACKAGE_SOURCE}
      FORCE True
      OPTIONS ${CPM_ARGS_OPTIONS}
    )
    cpm_export_variables(${CPM_ARGS_NAME})
    return()
  endif()

  # Check for available declaration
  if (NOT CPM_ARGS_FORCE AND NOT "${CPM_DECLARATION_${CPM_ARGS_NAME}}" STREQUAL "")
    set(declaration ${CPM_DECLARATION_${CPM_ARGS_NAME}})
    set(CPM_DECLARATION_${CPM_ARGS_NAME} "")
    CPMAddPackage(${declaration})
    cpm_export_variables(${CPM_ARGS_NAME})
    # checking again to ensure version and option compatibility
    CPMCheckIfPackageAlreadyAdded(${CPM_ARGS_NAME} "${CPM_ARGS_VERSION}" "${CPM_ARGS_OPTIONS}")
    return()
  endif()

  if(CPM_USE_LOCAL_PACKAGES OR CPM_LOCAL_PACKAGES_ONLY)
    cpm_find_package(${CPM_ARGS_NAME} "${CPM_ARGS_VERSION}" ${CPM_ARGS_FIND_PACKAGE_ARGUMENTS})

    if(CPM_PACKAGE_FOUND)
      cpm_export_variables(${CPM_ARGS_NAME})
      return()
    endif()

    if(CPM_LOCAL_PACKAGES_ONLY)
      message(SEND_ERROR "CPM: ${CPM_ARGS_NAME} not found via find_package(${CPM_ARGS_NAME} ${CPM_ARGS_VERSION})")
    endif()
  endif()

  CPMRegisterPackage("${CPM_ARGS_NAME}" "${CPM_ARGS_VERSION}")

  if (CPM_ARGS_OPTIONS)
    foreach(OPTION ${CPM_ARGS_OPTIONS})
      cpm_parse_option(${OPTION})
      set(${OPTION_KEY} ${OPTION_VALUE} CACHE INTERNAL "")
    endforeach()
  endif()

  if (DEFINED CPM_ARGS_GIT_TAG)
    set(PACKAGE_INFO "${CPM_ARGS_GIT_TAG}")
  elseif (DEFINED CPM_ARGS_SOURCE_DIR)
    set(PACKAGE_INFO "${CPM_ARGS_SOURCE_DIR}")
  else()
    set(PACKAGE_INFO "${CPM_ARGS_VERSION}")
  endif()

  if (DEFINED CPM_ARGS_DOWNLOAD_COMMAND)
    list(APPEND CPM_ARGS_UNPARSED_ARGUMENTS DOWNLOAD_COMMAND ${CPM_ARGS_DOWNLOAD_COMMAND})
  elseif (DEFINED CPM_ARGS_SOURCE_DIR)
    list(APPEND CPM_ARGS_UNPARSED_ARGUMENTS SOURCE_DIR ${CPM_ARGS_SOURCE_DIR})
  elseif (CPM_SOURCE_CACHE AND NOT CPM_ARGS_NO_CACHE)
    string(TOLOWER ${CPM_ARGS_NAME} lower_case_name)
    set(origin_parameters ${CPM_ARGS_UNPARSED_ARGUMENTS})
    list(SORT origin_parameters)
    string(SHA1 origin_hash "${origin_parameters}")
    set(download_directory ${CPM_SOURCE_CACHE}/${lower_case_name}/${origin_hash})
    list(APPEND CPM_ARGS_UNPARSED_ARGUMENTS SOURCE_DIR ${download_directory})
    if (EXISTS ${download_directory})
      # disable the download command to allow offline builds
      list(APPEND CPM_ARGS_UNPARSED_ARGUMENTS DOWNLOAD_COMMAND "${CMAKE_COMMAND}")
      set(PACKAGE_INFO "${download_directory}")
    else()
      # Enable shallow clone when GIT_TAG is not a commit hash.
      # Our guess may not be accurate, but it should guarantee no commit hash get mis-detected.
      if (NOT DEFINED CPM_ARGS_GIT_SHALLOW)
        cpm_is_git_tag_commit_hash("${CPM_ARGS_GIT_TAG}" IS_HASH)
        if (NOT ${IS_HASH})
          list(APPEND CPM_ARGS_UNPARSED_ARGUMENTS GIT_SHALLOW TRUE)
        endif()
      endif()

      # remove timestamps so CMake will re-download the dependency
      file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/_deps/${lower_case_name}-subbuild)
      set(PACKAGE_INFO "${PACKAGE_INFO} -> ${download_directory}")
    endif()
  endif()

  CPMCreateModuleFile(${CPM_ARGS_NAME} "CPMAddPackage(${ARGN})")

  if (CPM_PACKAGE_LOCK_ENABLED)
    if ((CPM_ARGS_VERSION AND NOT CPM_ARGS_SOURCE_DIR) OR CPM_INCLUDE_ALL_IN_PACKAGE_LOCK)
      cpm_add_to_package_lock(${CPM_ARGS_NAME} "${ARGN}")
    elseif(CPM_ARGS_SOURCE_DIR)
      cpm_add_comment_to_package_lock(${CPM_ARGS_NAME} "local directory")
    else()
      cpm_add_comment_to_package_lock(${CPM_ARGS_NAME} "${ARGN}")
    endif()
  endif()

  cpm_declare_fetch("${CPM_ARGS_NAME}" "${CPM_ARGS_VERSION}" "${PACKAGE_INFO}" "${CPM_ARGS_UNPARSED_ARGUMENTS}")
  cpm_fetch_package("${CPM_ARGS_NAME}" "${DOWNLOAD_ONLY}")
  cpm_get_fetch_properties("${CPM_ARGS_NAME}")

  SET(${CPM_ARGS_NAME}_ADDED YES)
  cpm_export_variables("${CPM_ARGS_NAME}")
endfunction()

# Fetch a previously declared package
macro(CPMGetPackage Name)
  if (DEFINED "CPM_DECLARATION_${Name}")
    CPMAddPackage(
      NAME ${Name}
    )
  else()
    message(SEND_ERROR "Cannot retrieve package ${Name}: no declaration available")
  endif()
endmacro()

# export variables available to the caller to the parent scope
# expects ${CPM_ARGS_NAME} to be set
macro(cpm_export_variables name)
  SET(${name}_SOURCE_DIR "${${name}_SOURCE_DIR}" PARENT_SCOPE)
  SET(${name}_BINARY_DIR "${${name}_BINARY_DIR}" PARENT_SCOPE)
  SET(${name}_ADDED "${${name}_ADDED}" PARENT_SCOPE)
endmacro()

# declares a package, so that any call to CPMAddPackage for the
# package name will use these arguments instead.
# Previous declarations will not be overriden.
macro(CPMDeclarePackage Name)
  if (NOT DEFINED "CPM_DECLARATION_${Name}")
    set("CPM_DECLARATION_${Name}" "${ARGN}")
  endif()
endmacro()

function(cpm_add_to_package_lock Name)
  if (NOT CPM_DONT_CREATE_PACKAGE_LOCK)
    file(APPEND ${CPM_PACKAGE_LOCK_FILE} "# ${Name}\nCPMDeclarePackage(${Name} \"${ARGN}\")\n")
  endif()
endfunction()

function(cpm_add_comment_to_package_lock Name)
  if (NOT CPM_DONT_CREATE_PACKAGE_LOCK)
    file(APPEND ${CPM_PACKAGE_LOCK_FILE} "# ${Name} (unversioned)\n# CPMDeclarePackage(${Name} \"${ARGN}\")\n")
  endif()
endfunction()

# includes the package lock file if it exists and creates a target
# `cpm-write-package-lock` to update it
macro(CPMUsePackageLock file)
  if (NOT CPM_DONT_CREATE_PACKAGE_LOCK)
    get_filename_component(CPM_ABSOLUTE_PACKAGE_LOCK_PATH ${file} ABSOLUTE)
    if(EXISTS ${CPM_ABSOLUTE_PACKAGE_LOCK_PATH})
      include(${CPM_ABSOLUTE_PACKAGE_LOCK_PATH})
    endif()
    if (NOT TARGET cpm-update-package-lock)
      add_custom_target(cpm-update-package-lock COMMAND ${CMAKE_COMMAND} -E copy ${CPM_PACKAGE_LOCK_FILE} ${CPM_ABSOLUTE_PACKAGE_LOCK_PATH})
    endif()
    set(CPM_PACKAGE_LOCK_ENABLED true)
  endif()
endmacro()

# registers a package that has been added to CPM
function(CPMRegisterPackage PACKAGE VERSION)
  list(APPEND CPM_PACKAGES ${PACKAGE})
  set(CPM_PACKAGES ${CPM_PACKAGES} CACHE INTERNAL "")
  set("CPM_PACKAGE_${PACKAGE}_VERSION" ${VERSION} CACHE INTERNAL "")
endfunction()

# retrieve the current version of the package to ${OUTPUT}
function(CPMGetPackageVersion PACKAGE OUTPUT)
  set(${OUTPUT} "${CPM_PACKAGE_${PACKAGE}_VERSION}" PARENT_SCOPE)
endfunction()

# declares a package in FetchContent_Declare
function (cpm_declare_fetch PACKAGE VERSION INFO)
  message(STATUS "${CPM_INDENT} adding package ${PACKAGE}@${VERSION} (${INFO})")

  if (${CPM_DRY_RUN})
    message(STATUS "${CPM_INDENT} package not declared (dry run)")
    return()
  endif()

  FetchContent_Declare(${PACKAGE}
    ${ARGN}
  )
endfunction()

# returns properties for a package previously defined by cpm_declare_fetch
function (cpm_get_fetch_properties PACKAGE)
  if (${CPM_DRY_RUN})
    return()
  endif()
  FetchContent_GetProperties(${PACKAGE})
  string(TOLOWER ${PACKAGE} lpackage)
  SET(${PACKAGE}_SOURCE_DIR "${${lpackage}_SOURCE_DIR}" PARENT_SCOPE)
  SET(${PACKAGE}_BINARY_DIR "${${lpackage}_BINARY_DIR}" PARENT_SCOPE)
endfunction()

# downloads a previously declared package via FetchContent
function (cpm_fetch_package PACKAGE DOWNLOAD_ONLY)
  if (${CPM_DRY_RUN})
    message(STATUS "${CPM_INDENT} package ${PACKAGE} not fetched (dry run)")
    return()
  endif()

  if(DOWNLOAD_ONLY)
    FetchContent_GetProperties(${PACKAGE})
    if(NOT ${PACKAGE}_POPULATED)
      FetchContent_Populate(${PACKAGE})
    endif()
  else()
    set(CPM_OLD_INDENT "${CPM_INDENT}")
    set(CPM_INDENT "${CPM_INDENT} ${PACKAGE}:")
    FetchContent_MakeAvailable(${PACKAGE})
    set(CPM_INDENT "${CPM_OLD_INDENT}")
  endif()
endfunction()

# splits a package option
function(cpm_parse_option OPTION)
  string(REGEX MATCH "^[^ ]+" OPTION_KEY ${OPTION})
  string(LENGTH ${OPTION} OPTION_LENGTH)
  string(LENGTH ${OPTION_KEY} OPTION_KEY_LENGTH)
  if (OPTION_KEY_LENGTH STREQUAL OPTION_LENGTH)
    # no value for key provided, assume user wants to set option to "ON"
    set(OPTION_VALUE "ON")
  else()
    math(EXPR OPTION_KEY_LENGTH "${OPTION_KEY_LENGTH}+1")
    string(SUBSTRING ${OPTION} "${OPTION_KEY_LENGTH}" "-1" OPTION_VALUE)
  endif()
  set(OPTION_KEY "${OPTION_KEY}" PARENT_SCOPE)
  set(OPTION_VALUE "${OPTION_VALUE}" PARENT_SCOPE)
endfunction()

# guesses the package version from a git tag
function(cpm_get_version_from_git_tag GIT_TAG RESULT)
  string(LENGTH ${GIT_TAG} length)
  if (length EQUAL 40)
    # GIT_TAG is probably a git hash
    SET(${RESULT} 0 PARENT_SCOPE)
  else()
    string(REGEX MATCH "v?([0123456789.]*).*" _ ${GIT_TAG})
    SET(${RESULT} ${CMAKE_MATCH_1} PARENT_SCOPE)
  endif()
endfunction()

# guesses if the git tag is a commit hash or an actual tag or a branch nane.
function(cpm_is_git_tag_commit_hash GIT_TAG RESULT)
  string(LENGTH "${GIT_TAG}" length)
  # full hash has 40 characters, and short hash has at least 7 characters.
  if (length LESS 7 OR length GREATER 40)
    SET(${RESULT} 0 PARENT_SCOPE)
  else()
    if (${GIT_TAG} MATCHES "^[a-fA-F0-9]+$")
      SET(${RESULT} 1 PARENT_SCOPE)
    else()
      SET(${RESULT} 0 PARENT_SCOPE)
    endif()
  endif()
endfunction()
