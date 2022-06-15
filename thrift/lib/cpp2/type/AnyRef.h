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
  using Ptr = detail::Ptr;

 public:
  AnyRef() noexcept = default;

  template <typename Tag, typename T>
  constexpr static AnyRef create(T&& val) {
    return {Tag{}, std::forward<T>(val)};
  }

  // Rebinds the AnyRef to another value (or void).
  template <typename Tag, typename T = native_type<Tag>>
  void reset(T&& val) {
    ptr_ = Ptr::create<Tag, T>(&val);
  }
  void reset() noexcept { ptr_ = {}; }

  // Returns true iff the referenced value is 'empty', and not serialized in a
  // 'terse' context.
  bool empty() const { return ptr_.empty(); }
  // Returns true iff the referenced value is identical to the given value.
  bool identical(const AnyRef& rhs) const { return ptr_.identical(rhs.ptr_); }

  // Sets the referenced value to it's intrinsic default (e.g. ignoring custom
  // field defaults).
  void clear() { ptr_.clear(); }

  // Type accessors.
  constexpr const Type& type() const { return ptr_.type().thriftType; }
  constexpr const std::type_info& typeId() const { return ptr_.type().cppType; }

  // Append to a list, string, etc.
  void append(AnyRef val) { ptr_.append(val.ptr_); }

  // Add to a set, number, etc.
  bool add(AnyRef val) { return ptr_.add(val.ptr_); }

  // Put a key-value pair, overwriting any existing entry in a map, struct, etc.
  //
  // Returns true if an existing value was replaced.
  bool put(FieldId id, AnyRef val) { return ptr_.put(id, val.ptr_); }
  bool put(AnyRef key, AnyRef val) { return ptr_.put(key.ptr_, val.ptr_); }
  bool put(const std::string& name, AnyRef val) {
    return put(asRef(name), val);
  }

  // Get by value.
  AnyRef get(AnyRef key) & { return getImpl(key); }
  AnyRef get(AnyRef key) const& { return getImpl(key, true); }
  AnyRef get(AnyRef key) && { return getImpl(key, false, true); }
  AnyRef get(AnyRef key) const&& { return getImpl(key, true, true); }

  // Get by field id.
  AnyRef get(FieldId id) & { return getImpl(id); }
  AnyRef get(FieldId id) const& { return getImpl(id, true); }
  AnyRef get(FieldId id) && { return getImpl(id, false, true); }
  AnyRef get(FieldId id) const&& { return getImpl(id, true, true); }

  // Get by name.
  AnyRef get(const std::string& name) & { return get(asRef(name)); }
  AnyRef get(const std::string& name) && { return get(asRef(name)); }
  AnyRef get(const std::string& name) const& { return get(asRef(name)); }
  AnyRef get(const std::string& name) const&& { return get(asRef(name)); }

  // Type-safe value accessors.
  template <typename Tag>
  constexpr const native_type<Tag>& as() const {
    return ptr_.as<native_type<Tag>>();
  }
  template <typename Tag>
  constexpr native_type<Tag>& mut() {
    return ptr_.mut<native_type<Tag>>();
  }
  template <typename Tag>
  constexpr const native_type<Tag>* tryAs() const noexcept {
    return ptr_.tryAs<native_type<Tag>>();
  }
  template <typename Tag>
  constexpr const native_type<Tag>* tryMut() const noexcept {
    return ptr_.tryMut<native_type<Tag>>();
  }

 private:
  Ptr ptr_;

  static AnyRef asRef(const std::string& name) {
    return AnyRef::create<type::string_t>(name);
  }

  constexpr explicit AnyRef(Ptr data) : ptr_(data) {}
  template <typename Tag, typename T>
  AnyRef(Tag, T&& val) : ptr_(Ptr::create<Tag>(std::forward<T>(val))) {}

  AnyRef getImpl(
      AnyRef key, bool ctxConst = false, bool ctxRvalue = false) const {
    return AnyRef{ptr_.get({}, &key.ptr_, ctxConst, ctxRvalue)};
  }
  AnyRef getImpl(
      FieldId id, bool ctxConst = false, bool ctxRvalue = false) const {
    return AnyRef{ptr_.get(id, nullptr, ctxConst, ctxRvalue)};
  }
};

} // namespace type
} // namespace thrift
} // namespace apache
