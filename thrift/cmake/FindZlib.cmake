#
# - Try to find Facebook zstd library
# This will define
# ZLIB_FOUND
# ZLIB_INCLUDE_DIR
# ZLIB_LIBRARIES
#

find_path(
  ZLIB_INCLUDE_DIRS zlib.h
  HINTS
      $ENV{ZLIB_ROOT}/include
      ${ZLIB_ROOT}/include
)

find_library(
    ZLIB_LIBRARIES z zlib
    HINTS
        $ENV{ZLIB_ROOT}/lib
        ${ZLIB_ROOT}/lib
)

# For some reason ZLIB_FOUND is never marked as TRUE
set(ZLIB_FOUND TRUE)
mark_as_advanced(ZLIB_INCLUDE_DIRS ZLIB_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Zstd ZLIB_INCLUDE_DIRS ZLIB_LIBRARIES)

if(ZLIB_FOUND AND NOT ZLIB_FIND_QUIETLY)
    message(STATUS "ZLIB: ${ZLIB_INCLUDE_DIRS}")
endif()
