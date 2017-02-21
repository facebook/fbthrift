# Find Cpp Mustache (Mstch)
#
# This will define:
# MSTCH_FOUND
# MSTCH_INCLUDE_DIRS
# MSTCH_LIBRARIES

find_path(
    MSTCH_INCLUDE_DIRS mstch/mstch.hpp
    HINTS
        $ENV{MSTCH_ROOT}/include
        ${MSTCH_ROOT}/include
)

find_library(
    MSTCH_LIBRARIES
    NAMES mstch
    HINTS
        $ENV{MSTCH_ROOT}/src
        ${MSTCH_ROOT}/src
)

mark_as_advanced(MSTCH_INCLUDE_DIRS MSTCH_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mstch MSTCH_LIBRARIES MSTCH_INCLUDE_DIRS)

if(MSTCH_FOUND AND NOT MSTCH_FIND_QUIETLY)
  message(STATUS "MSTCH: ${MSTCH_INCLUDE_DIRS}")
endif()
