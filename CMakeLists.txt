cmake_minimum_required(VERSION 3.1.3 FATAL_ERROR)

# Package information
set(PACKAGE_NAME "thrift")
project(${PACKAGE_NAME} C CXX)
set(CMAKE_CXX_STANDARD 14) #Requires CMake 3.1.3

# Name the top level directories
set(THRIFT_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_BUILD ${CMAKE_CURRENT_BINARY_DIR})

# Override default install path for `make install` step
set(CMAKE_INSTALL_PREFIX ${THRIFT_BUILD} CACHE PATH "current dir build" FORCE)

# Add root dir so qualified includes work. E.g. #include "thrift/compiler/$x.h"
include_directories(${THRIFT_HOME})
include_directories(${THRIFT_BUILD})

# Set directory of the Find$x.cmake files to properly include dependencies
set(CMAKE_MODULE_PATH "${THRIFT_HOME}/thrift/cmake" ${CMAKE_MODULE_PATH})

# Find required dependencies
find_package(OpenSSL REQUIRED)
if(MSVC)
  set(Boost_USE_STATIC_LIBS ON) #Force static lib in msvc
endif(MSVC)
find_package(
  Boost 1.54.0 REQUIRED #1.54.0 or greater
  COMPONENTS
    filesystem
    system
)
include_directories(${Boost_INCLUDE_DIRS})

# Enable modular builds
option(compiler_only "Build the Thrift Compiler only" OFF)
option(lib_only "Build the Thrift Libraries only" OFF)
if(compiler_only OR lib_only)
  set(build_all OFF)
else()
  set(build_all ON)
endif(compiler_only OR lib_only)

# Find required dependencies for thrift/compiler
if(compiler_only OR build_all)
  find_package(BISON REQUIRED)
  find_package(FLEX REQUIRED)
  find_package(Mstch REQUIRED)
  include_directories(${MSTCH_INCLUDE_DIRS})
  set(THRIFT_BINARY ${THRIFT_BUILD}/thrift/compiler/thrift-compiler)
endif(compiler_only OR build_all)

# Find required dependencies for thrift/lib
if(lib_only OR build_all)
  find_package(Folly REQUIRED)
  find_package(Glog REQUIRED)
  find_package(Zstd REQUIRED)
  find_package(Wangle REQUIRED)
  include_directories(
    ${FOLLY_INCLUDE_DIR}
    ${GLOG_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
  )
endif(lib_only OR build_all)

# Add the test dependencies
# To run tests: `make test`
if(enable_tests)
  find_package(PythonInterp REQUIRED)
  find_package(GTest REQUIRED)
  find_package(GMock REQUIRED)
  include_directories(
    ${GTEST_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
  )
  enable_testing()
endif(enable_tests)

# Create a generalized function for tests
function(thrift_gtest tname srcfile)
  add_executable("${tname}-t" ${srcfile})
  target_link_libraries(
    "${tname}-t"

    ${ARGN}
    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_BOTH_LIBRARIES}
    pthread
  )
  gtest_add_tests("${tname}-t" "" ${srcfile})
endfunction(thrift_gtest)

add_subdirectory(thrift)
