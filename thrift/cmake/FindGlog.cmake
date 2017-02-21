# Find Glog
#
# This will define:
#  GLOG_FOUND
#  GLOG_INCLUDE_DIRS
#  GLOG_LIBRARIES

find_path(
  GLOG_INCLUDE_DIRS glog/logging.h
  HINTS
      $ENV{GLOG_ROOT}/include
      ${GLOG_ROOT}/include
)

find_library(
  GLOG_LIBRARIES glog
  HINTS
      $ENV{GLOG_ROOT}/src
      ${GLOG_ROOT}/src
)

mark_as_advanced(GLOG_INCLUDE_DIRS GLOG_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Glog GLOG_INCLUDE_DIRS GLOG_LIBRARIES)

if(GLOG_FOUND AND NOT GLOG_FIND_QUIETLY)
    message(STATUS "GLOG: ${GLOG_INCLUDE_DIRS}")
endif()
