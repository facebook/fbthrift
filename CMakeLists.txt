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
# a consumer built with BUILD_SHARED_LIBS=ON, nor into the thrift-python
# extension .so files. PIC is cheap here, and keeping the archives static
# avoids the cross-dylib RTTI and undefined-symbol problems that building
# Thrift itself shared exposes.
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

option(
  THRIFT_RPC
  "Enable the RPC transport stack and its dependencies (fizz, wangle, mvfst)."
  ON)
option(THRIFT_BENCHMARKS "Enable benchmarks (requires google/benchmark)." ON)
option(THRIFT_TESTS "Enable tests (requires GTest)." OFF)
option(THRIFT_PY_DEPRECATED
       "Enable the deprecated thrift-py library; use thrift-python instead" ON)
option(THRIFT_PYTHON
       "Enable the thrift-python library and extensions (requires Cython)" OFF)

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

# Older folly installs export Threads::Threads without declaring it.
# Newer folly finds it itself, making this redundant.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

# Find required dependencies for thrift/lib
find_package(Gflags REQUIRED)
find_package(Glog REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Xxhash REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Zstd REQUIRED)

# folly lists here whatever it had to build into its own build tree. Such a
# dependency is absent from an installed folly, so it must not reach thrift's
# install interface either. The property is global, so it is readable here even
# though folly set it in the scope FetchContent added it in.
get_property(thrift_folly_build_local GLOBAL PROPERTY FOLLY_BUILD_LOCAL_TARGETS)
if ("OpenSSL::Crypto" IN_LIST thrift_folly_build_local)
  if (NOT CMAKE_SKIP_INSTALL_RULES)
    message(
      FATAL_ERROR
        "folly built OpenSSL into its build tree, so installing thrift would "
        "produce a package pointing at an OpenSSL that is not installed. "
        "Install OpenSSL, or configure with -DCMAKE_SKIP_INSTALL_RULES=ON to "
        "build without installing.")
  endif ()
  if (CMAKE_VERSION VERSION_LESS 3.26)
    message(
      FATAL_ERROR
        "Keeping folly's OpenSSL out of thrift's link interface needs the "
        "CMake 3.26 $<BUILD_LOCAL_INTERFACE:> generator expression. Install "
        "OpenSSL, or use CMake 3.26 or newer.")
  endif ()
  set(thrift_localize_openssl ON)
endif ()
if (THRIFT_RPC)
  find_package(fizz CONFIG REQUIRED)
  find_package(wangle CONFIG REQUIRED)
  find_package(mvfst CONFIG REQUIRED)
endif ()

# Add the test dependencies.
if (THRIFT_TESTS)
  find_package(GTest REQUIRED)
  include(GoogleTest)
  enable_testing()
endif ()

add_subdirectory(thrift)

# Confine folly's build-tree OpenSSL to the build interface, rather than
# spelling the generator expression at each target_link_libraries() call.
# Rewriting the interface leaves the private link line alone, so these still
# link it; it just stops being something an installed thrift asks for.
if (thrift_localize_openssl)
  foreach (target transport async thrifttype)
    if (NOT TARGET ${target})
      continue()
    endif ()
    get_target_property(link ${target} INTERFACE_LINK_LIBRARIES)
    if (NOT link)
      continue()
    endif ()
    set(patched "")
    foreach (dep IN LISTS link)
      if (dep STREQUAL "OpenSSL::Crypto")
        list(APPEND patched "$<BUILD_LOCAL_INTERFACE:OpenSSL::Crypto>")
      else ()
        list(APPEND patched "${dep}")
      endif ()
    endforeach ()
    set_property(TARGET ${target} PROPERTY INTERFACE_LINK_LIBRARIES ${patched})
  endforeach ()
endif ()

# include(CPack) snapshots the CPACK_* variables where it is called, so keep it
# after every target, install rule and CPACK_* setting.
if (NOT DEFINED CPACK_GENERATOR)
  set(CPACK_GENERATOR RPM)
endif ()
set(CPACK_RPM_PACKAGE_PROVIDES thrift)
include(CPack)
