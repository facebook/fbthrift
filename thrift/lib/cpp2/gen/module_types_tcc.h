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

#pragma once

#include <cstdint>
#include <memory>

#include <folly/Range.h>
#include <folly/Traits.h>

#include <thrift/lib/cpp/protocol/TType.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/NimbleProtocol.h>
#include <thrift/lib/cpp2/protocol/ProtocolReaderStructReadState.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp2/protocol/TableBasedSerializer.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>

namespace apache {
namespace thrift {

class BinaryProtocolReader;
class BinaryProtocolWriter;
class ComapctProtocolReader;
class CompactProtocolWriter;
class SimpleJSONProtocolReader;
class SimpleJSONProtocolWriter;

namespace detail {

template <typename T>
struct TccStructTraits;

template <class T>
auto make_mutable_smart_ptr(folly::tag_t<std::unique_ptr<T>>) {
  return std::make_unique<T>();
}

template <class T>
auto make_mutable_smart_ptr(folly::tag_t<std::shared_ptr<T>>) {
  return std::make_shared<T>();
}

template <class T>
auto make_mutable_smart_ptr(folly::tag_t<std::shared_ptr<T const>>) {
  return std::make_shared<T>();
}

} // namespace detail

} // namespace thrift
} // namespace apache
