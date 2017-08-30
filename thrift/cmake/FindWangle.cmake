#
# - Try to find Facebook WANGLE library
# This will define
# WANGLE_FOUND
# WANGLE_INCLUDE_DIR
# WANGLE_LIBRARIES
#

find_path(
  WANGLE_INCLUDE_DIRS wangle/acceptor/Acceptor.h
  HINTS
      $ENV{WANGLE_ROOT}/include
      ${WANGLE_ROOT}/include
)

find_library(
    WANGLE_LIBRARIES wangle
    HINTS
        $ENV{WANGLE_ROOT}/lib
        ${WANGLE_ROOT}/lib
)

mark_as_advanced(WANGLE_INCLUDE_DIRS WANGLE_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Wangle WANGLE_INCLUDE_DIRS WANGLE_LIBRARIES)

if(WANGLE_FOUND AND NOT WANGLE_FIND_QUIETLY)
    message(STATUS "WANGLE: ${WANGLE_INCLUDE_DIRS}")
endif()
