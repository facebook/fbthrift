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

add_subdirectory(common)
add_subdirectory(compiler)
add_subdirectory(annotation)
add_subdirectory(lib)

# Install CMake package configuration files for Thrift.
include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/FBThriftConfig.cmake.in FBThriftConfig.cmake
  INSTALL_DESTINATION ${THRIFT_INSTALL_CMAKEDIR}
  PATH_VARS THRIFT_INSTALL_CMAKEDIR CMAKE_INSTALL_INCLUDEDIR)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FBThriftConfig.cmake
        DESTINATION ${THRIFT_INSTALL_CMAKEDIR})
# xxhash, gflags, glog and zstd ship no CMake package config, so
# FBThriftConfig.cmake resolves them in MODULE mode. Install their Find modules
# next to the config (which prepends its own directory to CMAKE_MODULE_PATH) so
# downstream find_package(FBThrift) can define the imported targets that
# FBThriftTargets.cmake refers to. FindLibUnwind is here because FindGlog uses it.
install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindXxhash.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindZstd.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindGflags.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindGlog.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindLibUnwind.cmake
  DESTINATION ${THRIFT_INSTALL_CMAKEDIR})
install(
  EXPORT fbthrift-exports
  FILE FBThriftTargets.cmake
  NAMESPACE FBThrift::
  DESTINATION ${THRIFT_INSTALL_CMAKEDIR})

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
