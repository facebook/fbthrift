#
# - Try to find Facebook YARPL library
# This will define
# YARPL_FOUND
# YARPL_INCLUDE_DIR
# YARPL_LIBRARIES
#

find_path(
        YARPL_INCLUDE_DIRS yarpl/Flowable.h
        HINTS
        $ENV{YARPL_ROOT}/include
        ${YARPL_ROOT}/include
)

MESSAGE(STATUS "YARPL PATH: " $ENV{YARPL_ROOT})

find_library(
        YARPL_LIBRARIES yarpl
        HINTS
        $ENV{YARPL_ROOT}/lib
        ${YARPL_ROOT}/lib
)

mark_as_advanced(YARPL_INCLUDE_DIRS YARPL_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Yarpl YARPL_INCLUDE_DIRS YARPL_LIBRARIES)

if(YARPL_FOUND AND NOT YARPL_FIND_QUIETLY)
    message(STATUS "YARPL: ${YARPL_INCLUDE_DIRS}")
endif()
