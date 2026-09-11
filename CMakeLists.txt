# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.24)

project(
  THRIFT
  VERSION 1.0.0
  LANGUAGES CXX)

# Build rules should still use $<TARGET_FILE...> instead of these paths:
# multi-config generators add a per-config subdirectory they do not carry.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GNUInstallDirs)

# Not cached, so the default follows CMAKE_INSTALL_LIBDIR on a reconfigure.
if (NOT DEFINED THRIFT_INSTALL_CMAKEDIR)
  set(THRIFT_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/fbthrift")
endif ()

# Relative RPATHs keep the installation relocatable. Append, so a value from
# the command line or from fbcode_builder (which sets one on macOS) survives.
# Both entries go on everything: libraries resolve via the first, executables
# in bindir via the second, and the unused one is an empty search path.
if (NOT WIN32)
  if (APPLE)
    set(_rpath_origin "@loader_path")
  else ()
    set(_rpath_origin "$ORIGIN")
  endif ()
  file(RELATIVE_PATH _libdir_from_bindir "/${CMAKE_INSTALL_BINDIR}"
       "/${CMAKE_INSTALL_LIBDIR}")
  list(APPEND CMAKE_INSTALL_RPATH "${_rpath_origin}"
       "${_rpath_origin}/${_libdir_from_bindir}")
endif ()

# Thrift's libraries default to STATIC, and without PIC they cannot link into
# a consumer built with BUILD_SHARED_LIBS=ON. PIC is cheap here, and keeping
# the archives static avoids the cross-dylib RTTI and undefined-symbol
# problems that building Thrift itself shared exposes.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# The compile settings and the include directories for `#include <thrift/...>`
# and generated headers. PUBLIC targets also export the relocatable install
# path; PRIVATE ones do not.
function (thrift_configure_target target visibility)
  target_compile_features(${target} ${visibility} cxx_std_20)
  # GCC 10 needs this even under -std=c++20, and folly keys FOLLY_HAS_COROUTINES
  # off it. Inert from GCC 11 on.
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} ${visibility}
                           $<$<COMPILE_LANGUAGE:CXX>:-fcoroutines>)
  endif ()
  target_include_directories(
    ${target} ${visibility} $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>)
  if (NOT visibility STREQUAL "PRIVATE")
    target_include_directories(${target} ${visibility}
                               $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  endif ()
endfunction ()

# Prepend, so entries from a parent project or the command line survive.
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thrift/cmake"
     "${CMAKE_CURRENT_SOURCE_DIR}/build/fbcode_builder/CMake")

option(THRIFT_PY_DEPRECATED
       "Install the thrift/lib/py library as an FB Python archive manifest" ON)
option(thriftpy3
       "Include thrift-py3 library and extensions in the build, requires Cython"
       OFF)
option(
  thrift_python
  "Include thrift-python library and extensions in the build, requires Cython"
  OFF)

# thrift_python requires position-independent code so static libs can be
# linked into the extension .so files. Must precede the fetch blocks: this
# only seeds targets created after it is set.
if (thrift_python)
  set(CMAKE_POSITION_INDEPENDENT_CODE
      ON
      CACHE BOOL
            "Forced ON because thrift_python requires position-independent code"
            FORCE)
endif ()

# Fetch the archive that `manifest` pins, so a fetched dependency matches the
# version getdeps builds. `name` is the FetchContent content name. Variables
# the dependency reads can be set by the caller; a function body sees them.
# Always takes the manifest's .tar.gz entry, since CMake untars everywhere.
function (thrift_fetch_from_manifest name manifest)
  set(path
      "${CMAKE_CURRENT_SOURCE_DIR}/build/fbcode_builder/manifests/${manifest}")
  file(READ "${path}" text)
  if (NOT text MATCHES
      "url = (https://[^\r\n]+\\.tar\\.gz)[\r\n]+sha256 = ([0-9a-f]+)")
    message(FATAL_ERROR "no archive in ${path}")
  endif ()
  set(url "${CMAKE_MATCH_1}")
  set(sha256 "${CMAKE_MATCH_2}")
  message(STATUS "${name} not found, fetching ${url}")
  include(FetchContent)
  FetchContent_Declare(
    ${name}
    URL ${url}
    URL_HASH SHA256=${sha256})
  FetchContent_MakeAvailable(${name})
endfunction ()

# Fetch fmt if it is not installed.
find_package(fmt CONFIG QUIET)
if (NOT fmt_FOUND)
  # fmt defaults this off for a subproject, which would leave it out of every
  # export set and make install(EXPORT) fail.
  set(FMT_INSTALL ON)
  thrift_fetch_from_manifest(fmt fmt)
endif ()

# Fetch Boost if it is not installed.
find_package(Boost QUIET)
if (NOT Boost_FOUND)
  set(BOOST_ENABLE_CMAKE ON)
  # Thrift itself only needs Boost::headers, which the superproject always
  # defines. The list is what folly asks for.
  set(BOOST_INCLUDE_LIBRARIES context filesystem program_options regex)
  if (WIN32)
    list(APPEND BOOST_INCLUDE_LIBRARIES thread)
  endif ()
  thrift_fetch_from_manifest(Boost boost)
endif ()

# Fetch folly if it is not installed.
find_package(folly CONFIG QUIET)
if (NOT folly_FOUND)
  # The revision this repo is tested against, kept current by rev_sync.
  set(folly_rev_file
      "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/github_hashes/facebook/folly-rev.txt"
  )
  file(READ "${folly_rev_file}" folly_rev_text)
  if (NOT folly_rev_text MATCHES "Subproject commit ([0-9a-f]+)")
    message(FATAL_ERROR "no folly revision in ${folly_rev_file}")
  endif ()
  message(STATUS "folly not found, fetching ${CMAKE_MATCH_1}")
  include(FetchContent)
  FetchContent_Declare(
    folly
    GIT_REPOSITORY https://github.com/facebook/folly.git
    GIT_TAG ${CMAKE_MATCH_1})
  FetchContent_MakeAvailable(folly)
endif ()

find_package(OpenSSL REQUIRED)

option(
  THRIFT_RPC
  "Enable the RPC transport stack and its dependencies (fizz, wangle, mvfst)."
  ON)
option(THRIFT_BENCHMARKS "Enable benchmarks (requires google/benchmark)." ON)

# Find required dependencies for the Thrift compiler.
include_directories(${OPENSSL_INCLUDE_DIR})
set(THRIFT1 thrift1)
set(THRIFTCPP2 thriftcpp2)
include(ThriftLibrary.cmake)
install(FILES ThriftLibrary.cmake
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thrift)

# Find required dependencies for thrift/lib
find_package(Gflags REQUIRED)
find_package(Glog REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Zstd REQUIRED)
find_package(Xxhash REQUIRED)
if (THRIFT_RPC)
  find_package(fizz CONFIG REQUIRED)
  find_package(wangle CONFIG REQUIRED)
  find_package(mvfst CONFIG REQUIRED)
endif ()
# https://cmake.org/cmake/help/v3.9/module/FindThreads.html
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
include_directories(${LIBGFLAGS_INCLUDE_DIR} ${GLOG_INCLUDE_DIRS}
                    ${OPENSSL_INCLUDE_DIR} ${ZSTD_INCLUDE_DIRS}
                    ${Xxhash_INCLUDE_DIR})
add_definitions("-DTHRIFT_HAVE_LIBSNAPPY=0")

if (thriftpy3)
  find_package(PythonInterp 3.6 REQUIRED)
  find_package(PythonLibs 3 REQUIRED)
  find_package(Cython 0.28 REQUIRED)
endif ()

# Add the test dependencies.
if (enable_tests)
  find_package(PythonInterp REQUIRED)
  find_package(GTest REQUIRED)
  find_package(GMock REQUIRED)
  include_directories(${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS})
  enable_testing()
endif ()

add_subdirectory(thrift)

# include(CPack) snapshots the CPACK_* variables where it is called, so keep it
# after every target, install rule and CPACK_* setting.
if (NOT DEFINED CPACK_GENERATOR)
  set(CPACK_GENERATOR RPM)
endif ()
set(CPACK_RPM_PACKAGE_PROVIDES thrift)
include(CPack)
