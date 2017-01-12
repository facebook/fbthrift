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

# Add the test dependencies
# To run tests: `make test`
if(ENABLE_TESTS)
  find_package(GTest REQUIRED)
  find_package(GMock REQUIRED)
  include_directories(
    ${GTEST_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
  )

  enable_testing()
endif(ENABLE_TESTS)

# Create a generalized function for tests
function(thrift_gtest tname srcfile)
  if(ENABLE_TESTS)
    add_executable("${tname}-t" ${srcfile})
    target_link_libraries(
      "${tname}-t"

      ${ARGN}
      ${GTEST_BOTH_LIBRARIES}
      ${GMOCK_BOTH_LIBRARIES}
      pthread
    )
    gtest_add_tests("${tname}-t" "" ${srcfile})
  endif(ENABLE_TESTS)
endfunction(thrift_gtest)

add_subdirectory(thrift)
