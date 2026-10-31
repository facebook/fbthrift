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

include(ThriftGenerate)

# The exported and file names of the targets whose names differ from them, as
# <target>:<name>.
set(thrift_export_names
    thrift_core:thrift-core
    thrift_concurrency:concurrency
    thrift_runtime:runtime
    thrift_transport:transport
    thrift_async:async
    thrift_protocol:thriftprotocol
    thrift_rpc:thriftcpp2
    thrift_metadata:thriftmetadata
    thrift_rpc_metadata:rpcmetadata
    thrift_server_debug_info:serverdbginfo
    thrift_type:thrifttype
    thrift_type_rep:thrifttyperep
    thrift_any_rep:thriftanyrep
    thrift_frozen:thriftfrozen2
    thrift_VarintUtilsBench:VarintUtilsBench
    thrift_ProtocolBench:ProtocolBench)

# Adds thrift::<name> as an alias of the library target thrift_<name>, for
# projects that build Thrift from source.
function (thrift_add_alias target)
  string(REGEX REPLACE "^thrift_" "" name ${target})
  add_library(thrift::${name} ALIAS ${target})
endfunction ()

# Installs the targets into fbthrift-exports under their exported names.
# Projects that build fbthrift from source also get the FBThrift:: name
# FBThriftTargets.cmake gives each one, and the thrift:: alias of each library
# target named thrift_<name>.
function (thrift_install_targets)
  install(
    TARGETS ${ARGN}
    EXPORT fbthrift-exports
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  foreach (target IN LISTS ARGN)
    set(name ${target})
    foreach (entry IN LISTS thrift_export_names)
      if (entry MATCHES "^${target}:(.+)$")
        set(name ${CMAKE_MATCH_1})
        set_target_properties(${target} PROPERTIES EXPORT_NAME ${name}
                                                   OUTPUT_NAME ${name})
      endif ()
    endforeach ()
    get_target_property(type ${target} TYPE)
    if (type STREQUAL "EXECUTABLE")
      add_executable(FBThrift::${name} ALIAS ${target})
    else ()
      add_library(FBThrift::${name} ALIAS ${target})
      if (target MATCHES "^thrift_")
        thrift_add_alias(${target})
      endif ()
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
# FBThriftTargets.cmake refers to. FindLibUnwind is here because FindGlog uses
# it, and FindSodium because fizz's package config looks up Sodium without
# installing the module.
install(
  FILES ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindXxhash.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindZstd.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindGflags.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindGlog.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindLibUnwind.cmake
        ${PROJECT_SOURCE_DIR}/build/fbcode_builder/CMake/FindSodium.cmake
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

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ThriftGenerate.cmake
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
