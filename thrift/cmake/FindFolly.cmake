# Find Folly
#
# This will define:
# FOLLY_FOUND
# FOLLY_INCLUDE_DIR
# FOLLY_LIBRARY

find_path(
    FOLLY_INCLUDE_DIR folly/String.h
    HINTS
        $ENV{FOLLY_ROOT}/include
        ${FOLLY_ROOT}/include
)

find_library(
    FOLLY_LIBRARY
    NAMES folly
    HINTS
        $ENV{FOLLY_ROOT}/lib
        ${FOLLY_ROOT}/lib
)

mark_as_advanced(FOLLY_INCLUDE_DIR FOLLY_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FOLLY FOLLY_INCLUDE_DIR FOLLY_LIBRARY)

if(FOLLY_FOUND AND NOT FOLLY_FIND_QUIETLY)
    message(STATUS "FOLLY: ${FOLLY_INCLUDE_DIR}")
endif()
