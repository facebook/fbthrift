# - Try to find Google's GMock library
# This will define
# GMOCK_FOUND
# GMOCK_INCLUDE_DIRS
# GMOCK_LIBRARIES
# GMOCK_MAIN_LIBRARIES
# GMOCK_BOTH_LIBRARIES

find_path(GMOCK_INCLUDE_DIRS gmock/gmock.h
    HINTS
        $ENV{GMOCK_ROOT}/include
        ${GMOCK_ROOT}/include
)
mark_as_advanced(GMOCK_INCLUDE_DIRS)

find_library(GMOCK_LIBRARIES
    NAMES gmock
    HINTS
        ENV GMOCK_ROOT
        ${GMOCK_ROOT}
)
mark_as_advanced(GMOCK_LIBRARIES)

find_library(GMOCK_MAIN_LIBRARIES
    NAMES gmock_main
    HINTS
        ENV GMOCK_ROOT
        ${GMOCK_ROOT}
)
mark_as_advanced(GMOCK_LIBRARIES)

set(GMOCK_BOTH_LIBRARIES ${GMOCK_LIBRARIES} ${GMOCK_MAIN_LIBRARIES})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    GMock GMOCK_LIBRARIES GMOCK_INCLUDE_DIRS GMOCK_MAIN_LIBRARIES)
