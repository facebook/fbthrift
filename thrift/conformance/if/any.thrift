/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

namespace cpp2 apache.thrift.conformance
namespace php thrift
namespace py thrift.conformance.any
namespace py.asyncio thrift_asyncio.conformance.any
namespace py3 thrift.conformance
namespace java.swift org.apache.thrift.conformance
namespace go thrift.conformance.any

cpp_include "<folly/io/IOBuf.h>"

include "thrift/conformance/if/protocol.thrift"

const byte minTypeIdBytes = 16;
const byte maxTypeIdBytes = 32;

// Any encoded thrift value.
struct Any {
  // The unique name for this type.
  1: optional string type;
  // A prefix of the SHA3 hash of the unique type name.
  2: optional binary (cpp.type = "folly::fbstring") typeId;

  // The standard protocol used or StandardProtocol::None.
  3: protocol.StandardProtocol protocol;
  // The name of the custom protocol used, iff
  // protocol == StandardProtocol::None.
  4: optional string customProtocol;

  // The encoded value.
  // TODO(afuller): Consider switching to std::unique_ptr<folly::IOBuf> to make
  // moves cheaper (profile to see if this is better).
  5: binary (cpp.type = "folly::IOBuf") data;
}

// A configuration for a type used with Any.
//
// TODO(afuller): Use this as a structured annotation to enable Any support for
// a struct.
struct AnyType {
  // The name of the type.
  1: string name;

  // The list of alias for this type.
  2: list<string> aliases;

  // The default number of bytes to use in a typeId for this type.
  //
  // 0 indicates a type_id should never be used.
  // unset indicates that the implemnation should decided.
  3: optional byte typeIdBytes;
}
