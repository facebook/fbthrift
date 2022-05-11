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

#include <thrift/lib/cpp2/type/Any.h>
#include <thrift/lib/cpp2/type/AnyRef.h>
#include <thrift/lib/cpp2/type/AnyValue.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Protocol.h>
#include <thrift/lib/cpp2/type/detail/TypeRegistry.h>

namespace apache {
namespace thrift {
namespace type {

class TypeRegistry {
 public:
  // Returns the registry for types with generated code linked in.
  static const TypeRegistry& generated() {
    return detail::getGeneratedTypeRegistry();
  }

  // Store a value in an AnyData using the registered serializers.
  //
  // Throws std::out_of_range if no matching serializer has been registered.
  AnyData store(AnyRef value, const Protocol& protocol) const;
  template <StandardProtocol P>
  AnyData store(AnyRef value) const {
    return store(value, Protocol::get<P>());
  }
  template <typename Tag>
  AnyData store(const native_type<Tag>& value, const Protocol& protocol) const {
    return store(AnyRef::create<Tag>(value), protocol);
  }
  template <typename Tag, StandardProtocol P>
  AnyData store(const native_type<Tag>& value) const {
    return store(AnyRef::create<Tag>(value), Protocol::get<P>());
  }

  // Load a value from an AnyData using the registered serializers.
  //
  // Unless out refers to an empty std::any, the value is deserialized directly
  // into the referenced object, and the standard deserialization semantics will
  // hold. For example, set fields won't be cleared if not present in the
  // serialized value.
  //
  // Throws std::out_of_range if no matching serializer has been registered.
  // Throws std::bad_any_cast if value cannot be stored in out.
  [[noreturn]] void load(const AnyData& data, AnyRef out) const;
  AnyValue load(const AnyData& data) const;
};

} // namespace type
} // namespace thrift
} // namespace apache
