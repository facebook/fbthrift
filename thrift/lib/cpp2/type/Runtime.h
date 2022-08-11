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

#include <string>
#include <typeinfo>
#include <utility>

#include <thrift/lib/cpp/Field.h>
#include <thrift/lib/cpp2/op/detail/AnyOp.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/Type.h>
#include <thrift/lib/cpp2/type/detail/Runtime.h>

namespace apache {
namespace thrift {
namespace type {

// A light weight (pass-by-value), non-owning reference to a runtime Thrift
// value.
//
// Should typically be passed by value as it only holds two
// ponters; a pointer to the value being reference and a pointer to the static
// runtime metadata associated with the type of the value.
class Ref {
 public:
  template <typename Tag, typename T>
  static Ref create(T&& val) {
    return {Tag{}, std::forward<T>(val)};
  }

  Ref() noexcept = default;

  // Rebinds the Ref to another value (or void).
  template <typename Tag, typename T = native_type<Tag>>
  void reset(T&& val) {
    ptr_ = op::detail::createAnyPtr<Tag, T>(std::forward<T>(val));
  }
  void reset() noexcept { ptr_ = {}; }

  // Returns true iff the referenced value is 'empty', and not serialized in a
  // 'terse' context.
  bool empty() const { return ptr_.empty(); }
  // Returns true iff the referenced value is identical to the given value.
  bool identical(const Ref& rhs) const { return ptr_.identical(rhs.ptr_); }

  // Sets the referenced value to it's intrinsic default (e.g. ignoring custom
  // field defaults).
  void clear() { ptr_.clear(); }

  // Type accessors.
  const Type& type() const noexcept { return ptr_.type()->thriftType; }
  const std::type_info& typeId() const noexcept { return ptr_.type()->cppType; }

  // Append to a list, string, etc.
  void append(Ref val) { ptr_.append(val.ptr_); }

  // Add to a set, number, etc.
  bool add(Ref val) { return ptr_.add(val.ptr_); }

  // Put a key-value pair, overwriting any existing entry in a map, struct, etc.
  //
  // Returns true if an existing value was replaced.
  bool put(FieldId id, Ref val) { return ptr_.put(id, val.ptr_); }
  bool put(Ref key, Ref val) { return ptr_.put(key.ptr_, val.ptr_); }
  bool put(const std::string& name, Ref val) { return put(asRef(name), val); }

  // Get by value.
  Ref get(Ref key) & { return getImpl(key); }
  Ref get(Ref key) && { return getImpl(key, false, true); }
  Ref get(Ref key) const& { return getImpl(key, true); }
  Ref get(Ref key) const&& { return getImpl(key, true, true); }

  // Get by field id.
  Ref get(FieldId id) & { return getImpl(id); }
  Ref get(FieldId id) && { return getImpl(id, false, true); }
  Ref get(FieldId id) const& { return getImpl(id, true); }
  Ref get(FieldId id) const&& { return getImpl(id, true, true); }

  // Get by name.
  Ref get(const std::string& name) & { return get(asRef(name)); }
  Ref get(const std::string& name) && { return get(asRef(name)); }
  Ref get(const std::string& name) const& { return get(asRef(name)); }
  Ref get(const std::string& name) const&& { return get(asRef(name)); }

  // Type-safe value accessors.
  template <typename Tag>
  constexpr const native_type<Tag>& as() const {
    return ptr_.as<Tag>();
  }
  template <typename Tag>
  constexpr native_type<Tag>& mut() {
    return ptr_.mut<Tag>();
  }
  template <typename Tag>
  constexpr const native_type<Tag>* tryAs() const noexcept {
    return ptr_.tryAs<Tag>();
  }
  template <typename Tag>
  constexpr const native_type<Tag>* tryMut() const noexcept {
    return ptr_.tryMut<Tag>();
  }

 private:
  detail::Ptr ptr_;

  static Ref asRef(const std::string& name) {
    return Ref::create<type::string_t>(name);
  }

  explicit Ref(detail::Ptr data) noexcept : ptr_(data) {}
  template <typename Tag, typename T>
  Ref(Tag, T&& val)
      : ptr_(op::detail::createAnyPtr<Tag>(std::forward<T>(val))) {}

  Ref getImpl(Ref key, bool ctxConst = false, bool ctxRvalue = false) const {
    return Ref{ptr_.get({}, &key.ptr_, ctxConst, ctxRvalue)};
  }
  Ref getImpl(FieldId id, bool ctxConst = false, bool ctxRvalue = false) const {
    return Ref{ptr_.get(id, nullptr, ctxConst, ctxRvalue)};
  }
};

} // namespace type
} // namespace thrift
} // namespace apache
