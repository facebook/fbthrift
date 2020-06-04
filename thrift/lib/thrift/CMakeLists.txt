# Copyright (c) Facebook, Inc. and its affiliates.
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

thrift_generate(
  "reflection" #file_name
  "" #services
  "cpp2" #language
  "templates,no_metadata" #options
  "${CMAKE_CURRENT_SOURCE_DIR}" #file_path
  "${CMAKE_CURRENT_BINARY_DIR}" #output_path
  "thrift/lib/thrift" #include_prefix
)

thrift_generate(
  "metadata" #file_name
  "" #services
  "cpp2" #language
  "" #options
  "${CMAKE_CURRENT_SOURCE_DIR}" #file_path
  "${CMAKE_CURRENT_BINARY_DIR}" #output_path
  "thrift/lib/thrift" #include_prefix
)

thrift_generate(
  "frozen" #file_name
  "" #services
  "cpp2" #language
  "" #options
  "${CMAKE_CURRENT_SOURCE_DIR}" #file_path
  "${CMAKE_CURRENT_BINARY_DIR}" #output_path
  "thrift/lib/thrift" #include_prefix
)

thrift_generate(
  "RpcMetadata" #file_name
  "" #services
  "cpp2" #language
  "no_metadata" #options
  "${CMAKE_CURRENT_SOURCE_DIR}" #file_path
  "${CMAKE_CURRENT_BINARY_DIR}" #output_path
  "thrift/lib/thrift" #include_prefix
)

set_property(GLOBAL PROPERTY RpcMetadata-cpp2-SOURCES ${RpcMetadata-cpp2-SOURCES})
set_property(GLOBAL PROPERTY frozen-cpp2-SOURCES ${frozen-cpp2-SOURCES})
set_property(GLOBAL PROPERTY reflection-cpp2-SOURCES ${reflection-cpp2-SOURCES})
set_property(GLOBAL PROPERTY metadata-cpp2-SOURCES ${metadata-cpp2-SOURCES})
