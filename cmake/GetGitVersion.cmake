# - Returns a version string from Git tags
#
# This function inspects the annotated git tags for the project and returns a string
# into a CMake variable
#
#  get_git_version(<var>)
#
# - Example
#
# include(GetGitVersion)
# get_git_version(GIT_VERSION)
#
# Requires CMake 2.8.11+
find_package(Git)

if(__get_git_version)
  return()
endif()
set(__get_git_version INCLUDED)

function(get_git_version var)
  if(GIT_EXECUTABLE)
      execute_process(COMMAND ${GIT_EXECUTABLE} describe --abbrev=8
          RESULT_VARIABLE status
          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
          OUTPUT_VARIABLE FULL_GIT_VERSION
          ERROR_VARIABLE GIT_ERR)
      if(${status})
          message(WARNING "Git error ${status}:${GIT_ERR}")
      else()
          message("FULL_GIT_VERSION=${FULL_GIT_VERSION}")
          string(REGEX MATCH "v[0-9]+\\.[0-9]+\\.[0-9]+" GIT_VERSION ${FULL_GIT_VERSION})
      endif()

      # Work out if the repository is dirty
      execute_process(COMMAND ${GIT_EXECUTABLE} update-index -q --refresh
          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
          OUTPUT_QUIET
          ERROR_QUIET)
      execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --name-only HEAD --
          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
          OUTPUT_VARIABLE GIT_DIFF_INDEX
          ERROR_QUIET)
      string(COMPARE NOTEQUAL "${GIT_DIFF_INDEX}" "" GIT_DIRTY)
      if (${GIT_DIRTY})
          set(GIT_VERSION "${GIT_VERSION}-dirty")
      endif()
  else()
      message(WARNING "Git not found")
      set(GIT_VERSION "v0.0.0")
  endif()

  message("-- git Version: ${GIT_VERSION}")
  set(${var} ${GIT_VERSION} PARENT_SCOPE)
endfunction()
