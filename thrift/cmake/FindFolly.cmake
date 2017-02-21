# Find Folly
#
# This will define:
# FOLLY_FOUND
# FOLLY_INCLUDE_DIRS
# FOLLY_LIBRARIES

find_path(
    FOLLY_INCLUDE_DIRS folly/String.h
    HINTS
        $ENV{FOLLY_ROOT}/include
        ${FOLLY_ROOT}/include
)

find_library(
    FOLLY_LIBRARIES
    NAMES folly
    HINTS
        $ENV{FOLLY_ROOT}/lib
        ${FOLLY_ROOT}/lib
)

mark_as_advanced(FOLLY_INCLUDE_DIRS FOLLY_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FOLLY FOLLY_INCLUDE_DIRS FOLLY_LIBRARIES)

if(FOLLY_FOUND AND NOT FOLLY_FIND_QUIETLY)
    message(STATUS "FOLLY: ${FOLLY_INCLUDE_DIRS}")
endif()
