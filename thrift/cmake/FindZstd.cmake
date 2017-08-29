#
# - Try to find Facebook zstd library
# This will define
# ZSTD_FOUND
# ZSTD_INCLUDE_DIR
# ZSTD_LIBRARIES
#

find_path(
  ZSTD_INCLUDE_DIRS zstd.h
  HINTS
      $ENV{ZSTD_ROOT}/include
      ${ZSTD_ROOT}/include
)

find_library(
    ZSTD_LIBRARIES zstd
    HINTS
        $ENV{ZSTD_ROOT}/lib
        ${ZSTD_ROOT}/lib
)

mark_as_advanced(ZSTD_INCLUDE_DIRS ZSTD_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Zstd ZSTD_INCLUDE_DIRS ZSTD_LIBRARIES)

if(ZSTD_FOUND AND NOT ZSTD_FIND_QUIETLY)
    message(STATUS "ZSTD: ${ZSTD_INCLUDE_DIRS}")
endif()
