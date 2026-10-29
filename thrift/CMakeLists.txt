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

include(ThriftLibrary)

# Installs the targets into fbthrift-exports. Projects that build fbthrift from
# source also get the FBThrift:: name FBThriftTargets.cmake gives each one.
function (thrift_install_targets)
  install(
    TARGETS ${ARGN}
    EXPORT fbthrift-exports
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  foreach (target IN LISTS ARGN)
    get_target_property(type ${target} TYPE)
    if (type STREQUAL "EXECUTABLE")
      add_executable(FBThrift::${target} ALIAS ${target})
    else ()
      add_library(FBThrift::${target} ALIAS ${target})
    endif ()
  endforeach ()
endfunction ()

add_subdirectory(common)
add_subdirectory(compiler)
add_subdirectory(annotation)
add_subdirectory(lib)

# Install CMake package configuration files for Thrift.
include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/FBThriftConfig.cmake.in FBThriftConfig.cmake
  INSTALL_DESTINATION ${THRIFT_INSTALL_CMAKEDIR}
  PATH_VARS CMAKE_INSTALL_INCLUDEDIR)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FBThriftConfig.cmake
        DESTINATION ${THRIFT_INSTALL_CMAKEDIR})
# xxhash, gflags, glog and zstd ship no CMake package config, so
# FBThriftConfig.cmake resolves them in MODULE mode. Install their Find modules
# next to the config (which prepends its own directory to CMAKE_MODULE_PATH) so
# downstream find_package(FBThrift) can define the imported targets that
# FBThriftTargets.cmake refers to. FindLibUnwind is here because FindGlog uses it.
install(
  FILES ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindXxhash.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindZstd.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindGflags.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindGlog.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindLibUnwind.cmake
  DESTINATION ${THRIFT_INSTALL_CMAKEDIR})
install(
  EXPORT fbthrift-exports
  FILE FBThriftTargets.cmake
  NAMESPACE FBThrift::
  DESTINATION ${THRIFT_INSTALL_CMAKEDIR})

# For projects that build fbthrift from source: the FBTHRIFT_COMPILER and
# FBTHRIFT_INCLUDE_DIR that fbcode_builder's add_fbthrift_*_library() read.
set(FBTHRIFT_COMPILER FBThrift::thrift CACHE INTERNAL "thrift compiler target")
set(FBTHRIFT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}"
    CACHE INTERNAL "fbthrift include directory")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ThriftLibrary.cmake
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thrift)

install(
  DIRECTORY annotation ${CMAKE_CURRENT_SOURCE_DIR}/annotation
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thrift
  FILES_MATCHING
  PATTERN *.thrift)

install(
  DIRECTORY annotation ${CMAKE_CURRENT_BINARY_DIR}/annotation
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thrift
  FILES_MATCHING
  PATTERN *.h
  PATTERN *.tcc
  PATTERN CMakeFiles EXCLUDE)
