/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <folly/portability/GFlags.h>
#include <thrift/lib/cpp2/protocol/ProtocolGFlags.h>

FOLLY_GFLAGS_DEFINE_int32(
    thrift_protocol_max_depth,
    15000,
    "How many nested struct/list/set/map are allowed");

FOLLY_GFLAGS_DEFINE_int32(
    thrift_cpp2_protocol_reader_string_limit,
    0,
    "Limit on string size when deserializing thrift, 0 is no limit");
FOLLY_GFLAGS_DEFINE_int32(
    thrift_cpp2_protocol_reader_container_limit,
    0,
    "Limit on container size when deserializing thrift, 0 is no limit");

FOLLY_GFLAGS_DEFINE_bool(
    thrift_cpp2_simple_json_base64_allow_padding,
    true,
    "Allow '=' padding when decoding base64 encoded binary fields in "
    "SimpleJsonProtocol");

FOLLY_GFLAGS_DEFINE_bool(
    thrift_cpp2_debug_skip_list_indices,
    false,
    "Wether to skip indices when debug-printing lists (unless overridden)");
FOLLY_GFLAGS_DEFINE_int64(
    thrift_cpp2_debug_string_limit,
    256,
    "Limit on string size when debug-printing thrift, 0 is no limit");
