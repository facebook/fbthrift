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

#include <bitset>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include <folly/CPortability.h>
#include <folly/lang/Exception.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/detail/TypeInfo.h>

namespace apache {
namespace thrift {
namespace type {

// A light weight (pass-by-value), non-owning reference to a Thrift value.
//
// Should typically be passed by value as it only holds two
// ponters; a pointer to the value being reference and a pointer to the static
// runtime metadata associated with the type of the value.
//
// TODO(afuller): Merge this with AnyValue and AnyStruct to create the omega
// `Any` type.
class AnyRef {
 public:
  constexpr AnyRef() noexcept = default;

  template <typename Tag, typename T>
  constexpr static AnyRef create(T&& val) {
    return {Tag{}, std::forward<T>(val)};
  }

  // Rebinds the AnyRef to another value (or void).
  template <typename Tag, typename T = native_type<Tag>>
  void reset(T&& val) {
    ref_ = {&detail::getRefInfo<Tag, T>(), &val};
  }
  void reset() noexcept { ref_ = {}; }

  // Returns true iff the referenced value is 'empty', and not serialized in a
  // 'terse' context.
  bool empty() const { return ref_.empty(); }
  // Returns true iff the referenced value is identical to the given value.
  bool identical(const AnyRef& rhs) const { return ref_.identical(rhs.ref_); }

  // Sets the referenced value to it's intrinsic default (e.g. ignoring custom
  // field defaults).
  void clear() { ref_.clear(); }

  // Type accessors.
  constexpr const Type& type() const { return ref_.type().thriftType; }
  constexpr const std::type_info& typeId() const {
    return ref_.info->type.cppType;
  }

  // Append to a list, string, etc.
  void append(AnyRef val) { ref_.append(val.ref_); }

  // Add to a set, number, etc.
  bool add(AnyRef val) { return ref_.add(val.ref_); }

  // Put a key-value pair, overwriting any existing entry in a map, struct, etc.
  //
  // Returns true if an exisiting value was replaced.
  bool put(FieldId id, AnyRef val) { return ref_.put(id, val.ref_); }
  bool put(AnyRef key, AnyRef val) { return ref_.put(key.ref_, val.ref_); }
  bool put(const std::string& name, AnyRef val) {
    return put(AnyRef::create<string_t>(name), val);
  }

  // TODO(afuller): Add const access versions.
  // Get by key value.
  AnyRef get(AnyRef key) { return AnyRef{ref_.get({}, &key.ref_)}; }
  // Get by field id.
  AnyRef get(FieldId id) { return AnyRef{ref_.get(id, nullptr)}; }
  // Get by name.
  AnyRef get(const std::string& name) {
    return get(AnyRef::create<type::string_t>(name));
  }

  // Type-safe value accessors.
  template <typename Tag>
  constexpr const native_type<Tag>& as() const {
    return ref_.as<native_type<Tag>>();
  }
  template <typename Tag>
  constexpr native_type<Tag>& mut() {
    return ref_.mut<native_type<Tag>>();
  }
  template <typename Tag>
  constexpr const native_type<Tag>* tryAs() const noexcept {
    return ref_.tryAs<native_type<Tag>>();
  }
  template <typename Tag>
  constexpr const native_type<Tag>* tryMut() const noexcept {
    return ref_.tryMut<native_type<Tag>>();
  }

 private:
  detail::Ref ref_;

  constexpr explicit AnyRef(detail::Ref data) : ref_(data) {}
  template <typename Tag, typename T>
  AnyRef(Tag, T&& val) : ref_(detail::Ref::create<Tag>(std::forward<T>(val))) {}
};

} // namespace type
} // namespace thrift
} // namespace apache
