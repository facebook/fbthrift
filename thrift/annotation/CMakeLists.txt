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

add_library(thrift_annotation)
# The old name, kept for compatibility.
add_library(thriftannotation ALIAS thrift_annotation)
thrift_configure_target(thrift_annotation PUBLIC)
target_link_libraries(thrift_annotation PUBLIC Folly::folly)

foreach (
  name
  cpp
  hack
  java
  python
  go
  scope
  thrift)
  thrift_generate(
    ${name}.thrift
    LANGUAGE cpp2
    TARGET thrift_annotation
    OPTIONS templates no_metadata)
endforeach ()

install(
  TARGETS thrift_annotation
  EXPORT fbthrift-exports
  DESTINATION ${CMAKE_INSTALL_LIBDIR})
