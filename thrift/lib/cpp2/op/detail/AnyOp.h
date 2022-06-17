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
#include <thrift/lib/cpp2/type/detail/Ptr.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {
using Ptr = type::detail::Ptr;

// Type erased Thrift operator implementations.
template <typename Tag, typename T>
struct AnyOp : BaseAnyOp<Tag, T> {
  using type::detail::BaseErasedOp::unimplemented;
  [[noreturn]] static void append(void*, const Ptr&) { unimplemented(); }
  [[noreturn]] static bool add(void*, const Ptr&) { unimplemented(); }
  [[noreturn]] static bool put(void*, FieldId, const Ptr*, const Ptr&) {
    unimplemented();
  }
  [[noreturn]] static Ptr get(Ptr, FieldId, const Ptr*) { unimplemented(); }
};

template <typename Tag, typename T>
struct NumericOp : BaseAnyOp<Tag, T> {
  using Base = BaseAnyOp<Tag, T>;
  using Base::ref;
  using Base::same;

  static bool add(void* self, const Ptr& val) {
    ref(self) += same(val);
    return true;
  }
};

template <typename T>
struct AnyOp<type::bool_t, T> : NumericOp<type::bool_t, T> {};
template <typename T>
struct AnyOp<type::byte_t, T> : NumericOp<type::byte_t, T> {};
template <typename T>
struct AnyOp<type::i16_t, T> : NumericOp<type::i16_t, T> {};
template <typename T>
struct AnyOp<type::i32_t, T> : NumericOp<type::i32_t, T> {};
template <typename T>
struct AnyOp<type::i64_t, T> : NumericOp<type::i64_t, T> {};
template <typename T>
struct AnyOp<type::float_t, T> : NumericOp<type::float_t, T> {};
template <typename T>
struct AnyOp<type::double_t, T> : NumericOp<type::double_t, T> {};

template <typename ValTag, typename T>
struct AnyOp<type::list<ValTag>, T> : BaseAnyOp<type::list<ValTag>, T> {
  using Tag = type::list<ValTag>;
  using Base = BaseAnyOp<Tag, T>;
  using Base::ref;
  using Base::unimplemented;

  static decltype(auto) val(const Ptr& v) {
    return Base::template ref<ValTag>(v);
  }

  static void append(void* self, const Ptr& v) {
    ref(self).emplace_back(val(v));
  }
  [[noreturn]] static bool add(void*, const Ptr&) {
    unimplemented(); // TODO(afuller): Add if not already present.
  }
  [[noreturn]] static Ptr get(Ptr, FieldId, const Ptr*) {
    unimplemented(); // TODO(afuller): Get by index.
  }
};

template <typename KeyTag, typename T>
struct AnyOp<type::set<KeyTag>, T> : BaseAnyOp<type::set<KeyTag>, T> {
  using Tag = type::set<KeyTag>;
  using Base = BaseAnyOp<Tag, T>;
  using Base::ref;
  using Base::unimplemented;
  constexpr static decltype(auto) key(const Ptr& ptr) {
    return Base::template ref<KeyTag>(ptr);
  }

  static bool add(void* self, const Ptr& k) {
    return ref(self).emplace(key(k)).second;
  }

  [[noreturn]] static Ptr get(Ptr, FieldId, const Ptr*) {
    unimplemented(); // TODO(afuller): Get by key (aka contains).
  }
};

// Create a AnyOp-based Thrift pointer.
template <typename Tag, typename T = type::native_type<Tag>>
Ptr createAnyPtr(T&& val) {
  return {
      &type::detail::getTypeInfo<AnyOp<Tag, std::decay_t<T>>, Tag, T>(),
      // Note: const safety is validated at runtime.
      const_cast<std::decay_t<T>*>(&val),
      std::is_const_v<std::remove_reference_t<T>>,
      std::is_rvalue_reference_v<T>};
}

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
