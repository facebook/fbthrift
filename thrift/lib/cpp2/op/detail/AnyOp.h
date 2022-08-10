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

#include <thrift/lib/cpp2/op/detail/BaseOp.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/detail/RuntimeType.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {
using Ptr = type::detail::Ptr;

// Compile-time and type-erased Thrift operator implementations.
template <typename Tag, typename = void>
struct AnyOp : BaseAnyOp<Tag> {
  static_assert(type::is_concrete_v<Tag>, "");

  // TODO(afuller): Implement all Tags and remove runtime throwing fallback.
  using type::detail::BaseErasedOp::unimplemented;
  [[noreturn]] static void append(void*, const Ptr&) { unimplemented(); }
  [[noreturn]] static bool add(void*, const Ptr&) { unimplemented(); }
  [[noreturn]] static bool put(void*, FieldId, const Ptr*, const Ptr&) {
    unimplemented();
  }
  [[noreturn]] static Ptr get(void*, FieldId, const Ptr*) { unimplemented(); }
};

template <typename Tag>
struct NumericOp : BaseAnyOp<Tag> {
  using T = type::native_type<Tag>;
  using Base = BaseAnyOp<Tag>;
  using Base::ref;

  static bool add(T& self, const T& val) { return (self += val, true); }
  static bool add(void* s, const Ptr& v) { return add(ref(s), v.as<Tag>()); }
};

template <>
struct AnyOp<type::bool_t> : NumericOp<type::bool_t> {};
template <>
struct AnyOp<type::byte_t> : NumericOp<type::byte_t> {};
template <>
struct AnyOp<type::i16_t> : NumericOp<type::i16_t> {};
template <>
struct AnyOp<type::i32_t> : NumericOp<type::i32_t> {};
template <>
struct AnyOp<type::i64_t> : NumericOp<type::i64_t> {};
template <>
struct AnyOp<type::float_t> : NumericOp<type::float_t> {};
template <>
struct AnyOp<type::double_t> : NumericOp<type::double_t> {};

template <typename ValTag, typename Tag = type::list<ValTag>>
struct ListOp : BaseAnyOp<Tag> {
  using T = type::native_type<Tag>;
  using Base = BaseAnyOp<Tag>;
  using Base::ref;
  using Base::unimplemented;

  template <typename V = type::native_type<ValTag>>
  static void append(T& self, V&& val) {
    self.emplace_back(std::forward<V>(val));
  }
  static void append(void* s, const Ptr& v) { append(ref(s), v.as<ValTag>()); }

  template <typename V = type::native_type<ValTag>>
  [[noreturn]] static bool add(T&, V&&) {
    unimplemented(); // TODO(afuller): Add if not already present.
  }
  static bool add(void* s, const Ptr& v) { return add(ref(s), v.as<ValTag>()); }

  template <typename U>
  static decltype(auto) get(U&& self, size_t pos) {
    return folly::forward_like<U>(self.at(pos));
  }

  [[noreturn]] static Ptr get(void*, FieldId, const Ptr*) {
    unimplemented(); // TODO(afuller): Get by position.
  }
};

template <typename ValTag>
struct AnyOp<type::list<ValTag>> : ListOp<ValTag> {};

template <typename KeyTag, typename Tag = type::set<KeyTag>>
struct SetOp : BaseAnyOp<Tag> {
  using T = type::native_type<Tag>;
  using Base = BaseAnyOp<Tag>;
  using Base::ref;
  using Base::unimplemented;

  template <typename K = type::native_type<KeyTag>>
  static bool add(T& self, K&& key) {
    return self.emplace(std::forward<K>(key)).second;
  }
  static bool add(void* s, const Ptr& k) { return add(ref(s), k.as<KeyTag>()); }

  template <typename K = type::native_type<KeyTag>>
  static bool contains(const T& self, K&& key) {
    return self.find(std::forward<K>(key)) != self.end();
  }
  [[noreturn]] static Ptr get(void*, FieldId, const Ptr*) {
    unimplemented(); // TODO(afuller): Get by key (aka contains).
  }
};

template <typename KeyTag>
struct AnyOp<type::set<KeyTag>> : SetOp<KeyTag> {};

template <
    typename KeyTag,
    typename ValTag,
    typename Tag = type::map<KeyTag, ValTag>>
struct MapOp : BaseAnyOp<type::map<KeyTag, ValTag>> {
  using T = type::native_type<Tag>;
  using Base = BaseAnyOp<Tag>;
  using Base::bad_op;
  using Base::ref;
  using Base::unimplemented;

  template <
      typename K = type::native_type<KeyTag>,
      typename V = type::native_type<ValTag>>
  static bool put(T& self, K&& key, V&& val) {
    auto itr = self.find(key);
    if (itr == self.end()) {
      self.emplace(std::forward<K>(key), std::forward<V>(val));
      return false; // new entry.
    }
    itr->second = std::forward<V>(val);
    return true; // replacing existing.
  }

  static bool put(void* s, FieldId, const Ptr* k, const Ptr& v) {
    if (k == nullptr) {
      bad_op();
    }
    return put(ref(s), k->as<KeyTag>(), v.as<ValTag>());
  }

  [[noreturn]] static bool add(void*, const Ptr&) {
    // TODO(afuller): Add key/value pair, if key not already present.
    unimplemented();
  }

  [[noreturn]] static Ptr get(void*, FieldId, const Ptr*) {
    unimplemented(); // TODO(afuller): Get by key.
  }
};

template <typename KeyTag, typename ValTag>
struct AnyOp<type::map<KeyTag, ValTag>> : MapOp<KeyTag, ValTag> {};

// Create a AnyOp-based Thrift pointer.
template <typename Tag, typename T = type::native_type<Tag>>
Ptr createAnyPtr(T&& val) {
  return {
      {type::detail::getTypeInfo<AnyOp<Tag>, Tag, T>(),
       std::is_const_v<std::remove_reference_t<T>>,
       std::is_rvalue_reference_v<T>},
      // Note: const safety is validated at runtime.
      const_cast<std::decay_t<T>*>(&val),
  };
}

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
