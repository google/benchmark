find_path(
  Papi_INCLUDE_DIR
  NAMES papi.h)

find_library(Papi_LIBRARIES papi)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  Papi
  DEFAULT_MSG
  Papi_LIBRARIES Papi_INCLUDE_DIR)

mark_as_advanced(Papi_INCLUDE_DIR Papi_LIBRARIES)
