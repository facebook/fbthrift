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

#include <cstddef>
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

// A light weight (pass-by-value), non-owning *const* reference to a runtime
// Thrift value.
//
// Should typically be passed by value as it only holds two
// ponters; a pointer to the value being reference and a pointer to the static
// runtime metadata associated with the type of the value.
class ConstRef final : public detail::RuntimeBase {
  using Base = detail::RuntimeBase;

 public:
  template <typename Tag, typename T>
  static ConstRef create(T&& val) {
    return {Tag{}, std::forward<T>(val)}; // Preserve underlying ref type.
  }

  ConstRef() noexcept = default;
  // 'capture' any other runtime type.
  /* implicit */ ConstRef(const Base& other) noexcept : Base(other) {}

  // Rebinds the Ref to another value (or void).
  template <typename Tag, typename T = native_type<Tag>>
  void reset(T&& val) {
    Base::reset(op::detail::getAnyType<Tag, T>(), &val);
  }
  void reset() noexcept { Base::reset(); }

  // Get by value.
  ConstRef get(const Base& key) & { return Base::get(key); }
  ConstRef get(const Base& key) && { return Base::get(key, false, true); }
  ConstRef get(const Base& key) const& { return Base::get(key, true); }
  ConstRef get(const Base& key) const&& { return Base::get(key, true, true); }

  // Get by field id.
  ConstRef get(FieldId id) & { return Base::get(id); }
  ConstRef get(FieldId id) && { return Base::get(id, false, true); }
  ConstRef get(FieldId id) const& { return Base::get(id, true); }
  ConstRef get(FieldId id) const&& { return Base::get(id, true, true); }

  // Get by name.
  ConstRef get(const std::string& name) & { return get(asRef(name)); }
  ConstRef get(const std::string& name) && { return get(asRef(name)); }
  ConstRef get(const std::string& name) const& { return get(asRef(name)); }
  ConstRef get(const std::string& name) const&& { return get(asRef(name)); }

 private:
  friend class detail::Ptr;

  static ConstRef asRef(const std::string& name) {
    return create<type::string_t>(name);
  }

  template <typename Tag, typename T>
  ConstRef(Tag, T&& val) : Base(op::detail::getAnyType<Tag, T>(), &val) {}
};

// A light weight (pass-by-value), non-owning reference to a runtime Thrift
// value.
//
// Should typically be passed by value as it only holds two
// ponters; a pointer to the value being reference and a pointer to the static
// runtime metadata associated with the type of the value.
class Ref final : public detail::RuntimeBase {
  using Base = detail::RuntimeBase;

 public:
  template <typename Tag, typename T>
  static Ref create(T&& val) {
    return {Tag{}, std::forward<T>(val)};
  }

  Ref() noexcept = default;

  // Sets the referenced value to it's intrinsic default (e.g. ignoring custom
  // field defaults).
  void clear() { Base::clear(); }

  // Append to a list, string, etc.
  void append(const Base& val) { Base::append(val); }

  // Add to a set, number, etc.
  bool add(const Base& val) { return Base::add(val); }

  // Put a key-value pair, overwriting any existing entry in a map, struct, etc.
  //
  // Returns true if an existing value was replaced.
  bool put(FieldId id, const Base& val) { return Base::put(id, val); }
  bool put(const Base& key, const Base& val) { return Base::put(key, val); }
  bool put(const std::string& name, const Base& val) {
    return put(asRef(name), val);
  }

  // Get by value.
  Ref get(const Base& key) & { return *Base::get(key); }
  Ref get(const Base& key) && { return *Base::get(key, false, true); }
  ConstRef get(const Base& key) const& { return Base::get(key, true); }
  ConstRef get(const Base& key) const&& { return Base::get(key, true, true); }

  // Get by field id.
  Ref get(FieldId id) & { return *Base::get(id); }
  Ref get(FieldId id) && { return *Base::get(id, false, true); }
  ConstRef get(FieldId id) const& { return Base::get(id, true); }
  ConstRef get(FieldId id) const&& { return Base::get(id, true, true); }

  // Get by name.
  Ref get(const std::string& name) & { return get(asRef(name)); }
  Ref get(const std::string& name) && { return get(asRef(name)); }
  ConstRef get(const std::string& name) const& { return get(asRef(name)); }
  ConstRef get(const std::string& name) const&& { return get(asRef(name)); }

  // Throws on mismatch or if const.
  template <typename Tag>
  native_type<Tag>& mut() {
    return Base::mut<Tag>();
  }

  // Returns nullptr on mismatch or if const.
  template <typename Tag>
  native_type<Tag>* tryMut() noexcept {
    return Base::tryMut<Tag>();
  }

 private:
  friend class detail::Ptr;

  static ConstRef asRef(const std::string& name) {
    return ConstRef::create<type::string_t>(name);
  }

  explicit Ref(detail::Ptr data) noexcept : Base(data) {}
  template <typename Tag, typename T>
  Ref(Tag, T&& val) : Base(op::detail::getAnyType<Tag>(), &val) {}
};

namespace detail {
inline Ref Ptr::operator*() const noexcept {
  return Ref{*this};
}
} // namespace detail

// A runtime Thrift value that owns it's own memory.
//
// TODO(afuller): Store small values in-situ.
class Value : public detail::RuntimeBase {
  using Base = detail::RuntimeBase;

 public:
  template <typename Tag>
  static Value create() {
    return Value{Tag{}, nullptr};
  }
  template <typename Tag, typename T>
  static Value create(T&& val) {
    return {Tag{}, std::forward<T>(val)};
  }
  template <typename Tag>
  static Value create(std::unique_ptr<native_type<Tag>> val) {
    if (val == nullptr) {
      return {};
    }
    return Value{Tag{}, std::move(val)};
  }

  Value() noexcept = default; // A void/null value.
  Value(const Value& other) noexcept
      : Base(other.type_, other.type_->make(other.ptr_, false)) {}

  Value(Value&& other) noexcept : Base(other.type_, other.ptr_) {
    other.Base::reset();
  }
  ~Value() { reset(); }

  Value& operator=(const Value& other) noexcept {
    reset();
    Base::reset(other.type_, other.type_->make(other.ptr_, false));
    return *this;
  }

  Value& operator=(Value&& other) noexcept {
    reset();
    Base::reset(other.type_, other.ptr_);
    other.Base::reset();
    return *this;
  }

  void reset() {
    if (ptr_ != nullptr) {
      type_->delete_(ptr_);
      Base::reset();
    }
  }
  using Base::as;
  template <typename Tag>
  native_type<Tag>& as() & {
    return type_->as<native_type<Tag>>(ptr_);
  }
  template <typename Tag>
  native_type<Tag>&& as() && {
    return std::move(type_->as<native_type<Tag>>(ptr_));
  }

  // Returns nullptr on mismatch.
  template <typename Tag>
  native_type<Tag>* tryAs() noexcept {
    return type_->tryAs<native_type<Tag>>(ptr_);
  }
  using Base::tryAs;

  // Sets the referenced value to it's intrinsic default (e.g. ignoring custom
  // field defaults).
  void clear() { Base::clear(); }

  // Append to a list, string, etc.
  void append(const Base& val) { Base::append(val); }

  // Add to a set, number, etc.
  bool add(const Base& val) { return Base::add(val); }

  // Put a key-value pair, overwriting any existing entry in a map, struct, etc.
  //
  // Returns true if an existing value was replaced.
  bool put(FieldId id, const Base& val) { return Base::put(id, val); }
  bool put(const Base& key, const Base& val) { return Base::put(key, val); }
  bool put(const std::string& name, const Base& val) {
    return put(asRef(name), val);
  }

  // Get by value.
  Ref get(const Base& key) & { return *Base::get(key); }
  Ref get(const Base& key) && { return *Base::get(key, false, true); }
  ConstRef get(const Base& key) const& { return Base::get(key, true); }
  ConstRef get(const Base& key) const&& { return Base::get(key, true, true); }

  // Get by field id.
  Ref get(FieldId id) & { return *Base::get(id); }
  Ref get(FieldId id) && { return *Base::get(id, false, true); }
  ConstRef get(FieldId id) const& { return Base::get(id, true); }
  ConstRef get(FieldId id) const&& { return Base::get(id, true, true); }

  // Get by name.
  Ref get(const std::string& name) & { return get(asRef(name)); }
  Ref get(const std::string& name) && { return get(asRef(name)); }
  ConstRef get(const std::string& name) const& { return get(asRef(name)); }
  ConstRef get(const std::string& name) const&& { return get(asRef(name)); }

 private:
  template <typename Tag, typename T = native_type<Tag>>
  explicit Value(Tag, std::nullptr_t)
      : Base(
            op::detail::getAnyType<Tag, T>(),
            op::detail::getAnyType<Tag, T>()->make(nullptr, false)) {
    assert(ptr_ != nullptr);
  }
  template <typename Tag, typename T>
  Value(Tag, std::unique_ptr<T> val)
      : Base(op::detail::getAnyType<Tag, T>(), val.release()) {
    assert(ptr_ != nullptr);
  }
  template <typename Tag, typename T>
  Value(Tag, const T& val)
      : Base(
            op::detail::getAnyType<Tag, T>(),
            op::detail::getAnyType<Tag, T>()->make(&val, false)) {
    assert(ptr_ != nullptr);
  }
  template <typename Tag, typename T>
  Value(Tag, T&& val)
      : Base(
            op::detail::getAnyType<Tag, T>(),
            op::detail::getAnyType<Tag, T>()->make(&val, true)) {
    assert(ptr_ != nullptr);
  }

  static ConstRef asRef(const std::string& name) {
    return ConstRef::create<type::string_t>(name);
  }

  using Base::Base;
};

} // namespace type
} // namespace thrift
} // namespace apache
