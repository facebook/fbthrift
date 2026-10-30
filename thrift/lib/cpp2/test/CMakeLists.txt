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

add_executable(thrift_ProtocolBench ProtocolBench.cpp
                                    ../../thrift/detail/protocol.cpp)
# Generate Thrift files for this benchmark.
thrift_generate(thrift_ProtocolBench ProtocolBenchData.thrift LANGUAGE cpp2
                OPTIONS frozen2 layouts)
foreach (conf_name "protocol" "type" "any" "serialization" "patch_data"
                   "conformance")
  thrift_generate(
    thrift_ProtocolBench ../../../conformance/if/${conf_name}.thrift
    LANGUAGE cpp2 OPTIONS frozen2)
endforeach ()
find_package(benchmark CONFIG REQUIRED)
target_link_libraries(
  thrift_ProtocolBench PRIVATE thrift_frozen thrift_protocol thrift_type
                               Folly::folly benchmark::benchmark)

thrift_install_targets(thrift_ProtocolBench)
