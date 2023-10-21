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

#pragma once

#include <folly/portability/GFlags.h>

/**
 * All GFlags used by protocols are declared here and defined in
 * ProtocolGFlags.cpp.
 *
 * This is necessary because the CMake build system includes some of the
 * protocol sources in multiple libraries. When building with shared libraries,
 * this causes each library that contains the sources to register the flags,
 * which causes gflags to crash.
 *
 * To avoid the problem, we centralize all the flags and make that one CMake
 * library. Then each library which includes some protocol sources adds a
 * dependency on this library.
 */

FOLLY_GFLAGS_DECLARE_int32(thrift_protocol_max_depth);

FOLLY_GFLAGS_DECLARE_int32(thrift_cpp2_protocol_reader_string_limit);
FOLLY_GFLAGS_DECLARE_int32(thrift_cpp2_protocol_reader_container_limit);

FOLLY_GFLAGS_DECLARE_bool(thrift_cpp2_simple_json_base64_allow_padding);

FOLLY_GFLAGS_DECLARE_bool(thrift_cpp2_debug_skip_list_indices);
FOLLY_GFLAGS_DECLARE_int64(thrift_cpp2_debug_string_limit);
