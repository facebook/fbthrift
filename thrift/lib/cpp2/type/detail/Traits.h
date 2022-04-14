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

#include <map>
#include <set>
#include <string>
#include <vector>

#include <fatal/type/slice.h>
#include <fatal/type/sort.h>
#include <folly/Traits.h>
#include <thrift/lib/cpp2/Adapt.h>
#include <thrift/lib/cpp2/type/AnyType.h>
#include <thrift/lib/cpp2/type/BaseType.h>
#include <thrift/lib/cpp2/type/ThriftType.h>

namespace apache {
namespace thrift {
namespace type {
namespace detail {

// All the traits for the given tag.
template <typename Tag>
struct traits {
  // No types to declare for non concrete types.
  static_assert(is_abstract_v<Tag>, "");
};

// Resolves the concrete template type when paramaterizing the given template,
// with the standard types of the give Tags.
template <template <typename...> class TemplateT, typename... Tags>
using standard_template_t = TemplateT<typename traits<Tags>::standard_type...>;

// Resolves the concrete template type when paramaterizing the given template,
// with the native types of the give Tags.
template <template <typename...> class TemplateT, typename... Tags>
using native_template_t = TemplateT<typename traits<Tags>::native_type...>;
template <template <typename...> class TemplateT, typename... Tags>
struct native_template {
  using type = native_template_t<TemplateT, Tags...>;
};

template <typename... Tags>
struct types {
  static constexpr bool contains(BaseType baseType) {
    for (bool i : {(base_type_v<Tags> == baseType)...}) {
      if (i) {
        return true;
      }
    }
    return false;
  }

  template <typename Tag>
  static constexpr bool contains() {
    return contains(base_type_v<Tag>);
  }

  static bool contains(const AnyType& type) {
    return contains(type.base_type());
  }

  // The Ith type.
  template <size_t I>
  using at = typename fatal::at<types, I>;

  // Converts the type list to a type list of the given types.
  template <template <typename...> class T>
  using as = T<Tags...>;

  template <typename F>
  using filter = typename fatal::filter<types, F>;

  template <typename CTag>
  using of = filter<type::bound::is_a<CTag>>;
};

template <typename Ts, typename Tag, typename R = void>
using if_contains = std::enable_if_t<Ts::template contains<Tag>(), R>;

// The (mixin) traits all concrete types (_t suffix) define.
//
// All concrete types have an associated set of standard types and a native type
// (which by default is just the first standard type).
template <typename StandardType, typename NativeType = StandardType>
struct concrete_type {
  using standard_type = StandardType;
  using native_type = NativeType;
};
struct nonconcrete_type {};

template <template <typename...> class T, typename ValTag>
using parameterized_type = folly::conditional_t<
    is_concrete_v<ValTag>,
    concrete_type<standard_template_t<T, ValTag>, native_template_t<T, ValTag>>,
    nonconcrete_type>;

template <template <typename...> class T, typename KeyTag, typename ValTag>
using parameterized_kv_type = folly::conditional_t<
    is_concrete_v<KeyTag> && is_concrete_v<ValTag>,
    concrete_type<
        standard_template_t<T, KeyTag, ValTag>,
        native_template_t<T, KeyTag, ValTag>>,
    nonconcrete_type>;

template <>
struct traits<void_t> : concrete_type<void> {};

// Type traits for all primitive types.
template <>
struct traits<bool_t> : concrete_type<bool> {};
template <>
struct traits<byte_t> : concrete_type<int8_t> {};
template <>
struct traits<i16_t> : concrete_type<int16_t> {};
template <>
struct traits<i32_t> : concrete_type<int32_t> {};
template <>
struct traits<i64_t> : concrete_type<int64_t> {};
template <>
struct traits<float_t> : concrete_type<float> {};
template <>
struct traits<double_t> : concrete_type<double> {};
template <>
struct traits<string_t> : concrete_type<std::string> {};
template <>
struct traits<binary_t> : concrete_type<std::string> {};

// Traits for enums.
template <typename E>
struct traits<enum_t<E>> : concrete_type<E> {};

// Traits for structs.
template <typename T>
struct traits<struct_t<T>> : concrete_type<T> {};

// Traits for unions.
template <typename T>
struct traits<union_t<T>> : concrete_type<T> {};

// Traits for excpetions.
template <typename T>
struct traits<exception_t<T>> : concrete_type<T> {};

// Traits for lists.
template <typename ValTag>
struct traits<type::list<ValTag>> : parameterized_type<std::vector, ValTag> {};

// Traits for sets.
template <typename KeyTag>
struct traits<set<KeyTag>> : parameterized_type<std::set, KeyTag> {};

// Traits for maps.
template <typename KeyTag, typename ValTag>
struct traits<map<KeyTag, ValTag>>
    : parameterized_kv_type<std::map, KeyTag, ValTag> {};

// Traits for adapted types.
//
// Adapted types are concrete and have an adapted native_type.
template <typename Adapter, typename Tag>
struct traits<adapted<Adapter, Tag>>
    : concrete_type<
          typename traits<Tag>::standard_type,
          adapt_detail::adapted_t<Adapter, typename traits<Tag>::native_type>> {
};

// Traits for cpp_type types.
//
// cpp_type types are concrete and have special native_type.
template <typename T, typename Tag>
struct traits<cpp_type<T, Tag>>
    : concrete_type<typename traits<Tag>::standard_type, T> {};

} // namespace detail
} // namespace type
} // namespace thrift
} // namespace apache
