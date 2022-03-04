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

#include <utility>
#include <thrift/conformance/cpp2/internal/Object.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp/protocol/TType.h>

namespace apache::thrift::conformance {

// Creates a Value struct for the given value.
//
// TT: The thrift type to use, for example
// apache::thrift::conformance::type::binary_t.
template <typename TT, typename T>
Value asValueStruct(T&& value) {
  Value result;
  detail::ValueHelper<TT>::set(result, std::forward<T>(value));
  return result;
}

// Schemaless deserialization of thrift serialized data
// into conformance::Object
// Protocol: protocol to use eg. apache::thrift::BinaryProtocolReader
// buf: serialized payload
// Works for binary, compact and json protocol. Does not work for SimpleJson
// protocol as it does not save fieldID and field type information in serialized
// data. There is no difference in serialized data for binary and string field
// for protocols except Json protocol. In Json protocol, binary field value is
// base64 encoded, while a string will be written as is. This method cannot not
// differentiate between binary and string field as both are marked as string
// field type in serialized data. Binary fields are saved in stringValue field
// of Object instead of binaryValue field.
template <class Protocol>
Object parseObject(const folly::IOBuf& buf) {
  Protocol prot;
  prot.setInput(&buf);
  auto result = detail::parseValue(prot, protocol::T_STRUCT);
  return *result.objectValue_ref();
}

// Schemaless serialization of conformance::Object
// into thrift serialization protocol
// Protocol: protocol to use eg. apache::thrift::BinaryProtocolWriter
// obj: object to be serialized
// Serialized output is same schema based serialization except when struct
// contains an empty list, set or map
template <class Protocol>
std::unique_ptr<folly::IOBuf> serializeObject(const Object& obj) {
  Protocol prot;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  prot.setOutput(&queue);
  Value val;
  val.objectValue_ref() = obj;
  detail::serializeValue(prot, val);
  return queue.move();
}

} // namespace apache::thrift::conformance
