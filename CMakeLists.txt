cmake_minimum_required(VERSION 3.1.3 FATAL_ERROR)

# Name the top level directories
set(THRIFT_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_BUILD ${CMAKE_CURRENT_BINARY_DIR})

# Add root dir so qualified includes work. E.g. #include "thrift/compiler/$x.h"
include_directories(${THRIFT_HOME})

# Override default install path for `make install` step
set(CMAKE_INSTALL_PREFIX ${THRIFT_BUILD} CACHE PATH "current dir build" FORCE)

# package information
set(PACKAGE_NAME "thrift_compiler")

project(${PACKAGE_NAME} C CXX)
set(CMAKE_CXX_STANDARD 11) #Requires CMake 3.1.3

# Add non-standard external dependencies
add_subdirectory(${THRIFT_HOME}/external/mstch)

# Set directory of the Find$x.cmake files to properly include dependencies
set(CMAKE_MODULE_PATH "${THRIFT_HOME}/thrift/cmake" ${CMAKE_MODULE_PATH})
# Find required dependencies
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Folly REQUIRED)
find_package(Glog REQUIRED)

if(MSVC)
  set(Boost_USE_STATIC_LIBS ON) #Force static lib in msvc
endif(MSVC)
find_package(
  Boost 1.54.0 REQUIRED #1.54.0 or greater
  COMPONENTS
    filesystem
    system
)

# Add the required dependency include paths
include_directories(
  ${mstch_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_subdirectory(thrift)
