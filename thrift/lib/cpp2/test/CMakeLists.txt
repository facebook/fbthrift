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

# This CMakeLists.txt takes care of building a selection of benchmarks and tests
# in this folder. Currently, it only builds ProtocolBench with its dependencies.

if (NOT THRIFT_BENCHMARKS)
  return()
endif ()

# Generate Thrift files for this benchmark.
thrift_generate(ProtocolBenchData.thrift LANGUAGE cpp2 OPTIONS frozen2 layouts
                OUT_SOURCES bench_data_sources)

set(GENERATED_THRIFT_SOURCES
    ${bench_data_sources} ${protocol_sources}
    ${protocol_detail_sources} ${field_mask_sources}
    ${frozen_sources})
set(SOURCE_FILES ProtocolBench.cpp)
set(ADDITIONAL_SOURCE_FILES ../../thrift/detail/protocol.cpp
                            ../../thrift/TypeToMaskAdapter.cpp)

# Generate conformance Thrift files.
foreach (conf_name "protocol" "type" "any" "serialization" "patch_data"
                   "conformance")
  thrift_generate(
    ../../../conformance/if/${conf_name}.thrift
    LANGUAGE cpp2
    OPTIONS frozen2
    NAME ${conf_name}conformance
    OUT_SOURCES conformance_sources)
  set(GENERATED_THRIFT_SOURCES ${GENERATED_THRIFT_SOURCES}
                               ${conformance_sources})
endforeach ()

foreach (GENERATED ${GENERATED_THRIFT_SOURCES})
  set_source_files_properties(${GENERATED} PROPERTIES GENERATED TRUE)
endforeach ()

add_executable(ProtocolBench ${SOURCE_FILES} ${ADDITIONAL_SOURCE_FILES}
                             ${GENERATED_THRIFT_SOURCES})
find_package(benchmark CONFIG REQUIRED)
target_link_libraries(ProtocolBench thriftfrozen2 thriftprotocol thrifttype
                      Folly::folly benchmark::benchmark)

install(
  TARGETS ProtocolBench
  EXPORT fbthrift-exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
