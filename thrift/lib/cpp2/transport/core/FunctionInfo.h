/*
 * Copyright 2017-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdint.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <string>

namespace apache {
namespace thrift {

using FunctionName = std::string;

enum FunctionKind {
  SINGLE_REQUEST_SINGLE_RESPONSE = 0,
  SINGLE_REQUEST_NO_RESPONSE = 1,
  STREAMING_REQUEST_SINGLE_RESPONSE = 2,
  STREAMING_REQUEST_NO_RESPONSE = 2,
  SINGLE_REQUEST_STREAMING_RESPONSE = 4,
  STREAMING_REQUEST_STREAMING_RESPONSE = 5,
};

/**
 * Encapsulates the metadata describing a specific Thrift function.
 */
struct FunctionInfo {
  FunctionName name;
  FunctionKind kind;
  // This is an unique identification of a particular RPC.  It is not
  // always necessary.  For example, if there is a separate channel
  // object for each RPC, the channel object serves as the unique
  // identification and seqId can be set to 0 and ignored.
  uint32_t seqId;
  apache::thrift::protocol::PROTOCOL_TYPES protocol;
};

} // namespace thrift
} // namespace apache
