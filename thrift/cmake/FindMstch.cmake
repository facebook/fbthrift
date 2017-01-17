# - Try to find Mustache Cpp (Mstch) library
# This will define
# MSTCH_FOUND
# MSTCH_INCLUDE_DIRS
# MSTCH_LIBRARIES

find_path(
    MSTCH_INCLUDE_DIRS mstch/mstch.hpp
    HINTS
        $ENV{MSTCH_ROOT}/include
        ${MSTCH_ROOT}/include
)
mark_as_advanced(MSTCH_INCLUDE_DIRS)

find_library(
    MSTCH_LIBRARIES
    NAMES mstch
    HINTS
        $ENV{MSTCH_ROOT}/src
        ${MSTCH_ROOT}/src
)
mark_as_advanced(MSTCH_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mstch MSTCH_LIBRARIES MSTCH_INCLUDE_DIRS)
