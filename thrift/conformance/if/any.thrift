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

// Any encoded thrift value.
struct Any {
  // The unique name for this type.
  1: string type;

  // The protocol used to encode the value.
  2: protocol.Protocol protocol;

  // The encoded value.
  // TODO(afuller): Consider switching to std::unique_ptr<folly::IOBuf> to make
  // moves cheaper (profile to see if this is better).
  3: binary (cpp.type = "folly::IOBuf") data;
}
