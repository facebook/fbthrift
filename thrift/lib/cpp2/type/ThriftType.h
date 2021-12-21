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

#include <type_traits>

#include <thrift/lib/cpp2/type/Tag.h>

// Helpers for working with thrift type tags.
namespace apache::thrift::type {

// If a given type tag refers to concrete type and not a class of types.
//
// For example:
//     is_concrete_v<byte_t> -> true
//     is_concrete_v<list_c> -> false
//     is_concrete_v<list<byte_t>> -> true
//     is_concrete_v<list<struct_c>> -> false
template <typename Tag>
constexpr bool is_concrete_v = false;

// If a given Thrift type tag is wellformed.
//
// For example:
//     is_thrift_type_tag_v<int> -> false
//     is_thrift_type_tag_v<byte_t> -> true
//     is_thrift_type_tag_v<list_c> -> true
//     is_thrift_type_tag_v<list<Foo>> -> false
//     is_thrift_type_tag_v<list<byte_t>> -> true
//     is_thrift_type_tag_v<list<struct_c>> -> true
template <typename Tag>
constexpr bool is_thrift_type_tag_v = is_concrete_v<Tag>;

// If a given Thrift type tag is not concrete.
//
// For example:
//     is_abstract_v<int> -> false
//     is_abstract_v<byte_t> -> false
//     is_abstract_v<list_c> -> true
//     is_abstract_v<list<Foo>> -> false
//     is_abstract_v<list<byte_t>> -> false
//     is_abstract_v<list<struct_c>> -> true
template <typename Tag>
constexpr bool is_abstract_v = is_thrift_type_tag_v<Tag> && !is_concrete_v<Tag>;

namespace detail {
template <typename... Tags>
constexpr void checkTags() {
  static_assert((is_thrift_type_tag_v<Tags> && ...));
}
} // namespace detail

// Is `true` iff `Tag` is in the type class, `CTag`.
//
// For example:
//     is_a_v<void_t, integral_c> -> false
//     is_a_v<i32_t, integral_c> -> true
//     is_a_v<i32_t, i32_t> -> true
//     is_a_v<i32_t, i64_t> -> false
//     is_a_v<list<i64_t>, list<i64_t>> -> true
//     is_a_v<list<integral_c>, list_c> -> true
//     is_a_v<list<i64_t>, list_c> -> true
//     is_a_v<list<i64_t>, list<integral_c>> -> true
//     is_a_v<list<i64_t>, list<i64_t>> -> true
template <typename Tag, typename CTag>
constexpr bool is_a_v =
    (detail::checkTags<Tag, CTag>(), std::is_base_of_v<CTag, Tag>);

// Helpers to enable/disable declarations based on if a type tag matches
// a constraint.
template <typename Tag, typename R = void, typename...>
using if_concrete = std::enable_if_t<is_concrete_v<Tag>, R>;
template <typename Tag, typename R = void, typename...>
using if_not_concrete = std::enable_if_t<is_abstract_v<Tag>, R>;
template <typename CTag, typename Tag, typename R = void, typename...>
using if_is_a = std::enable_if_t<is_a_v<CTag, Tag>, R>;

// Helpers for applying the conditions.
namespace bound {
struct is_concrete {
  template <typename Tag>
  using apply = std::bool_constant<type::is_concrete_v<Tag>>;
};
struct is_thrift_type_tag {
  template <typename Tag>
  using apply = std::bool_constant<type::is_thrift_type_tag_v<Tag>>;
};
struct is_abstract {
  template <typename Tag>
  using apply = std::bool_constant<type::is_abstract_v<Tag>>;
};
template <typename CTag>
struct is_a {
  template <typename Tag>
  using apply = std::bool_constant<type::is_a_v<Tag, CTag>>;
};
} // namespace bound

////
// Implemnation details

template <>
constexpr inline bool is_concrete_v<void_t> = true;
template <>
constexpr inline bool is_concrete_v<bool_t> = true;
template <>
constexpr inline bool is_concrete_v<byte_t> = true;
template <>
constexpr inline bool is_concrete_v<i16_t> = true;
template <>
constexpr inline bool is_concrete_v<i32_t> = true;
template <>
constexpr inline bool is_concrete_v<i64_t> = true;
template <>
constexpr inline bool is_concrete_v<float_t> = true;
template <>
constexpr inline bool is_concrete_v<double_t> = true;
template <>
constexpr inline bool is_concrete_v<string_t> = true;
template <>
constexpr inline bool is_concrete_v<binary_t> = true;

template <typename T>
constexpr inline bool is_concrete_v<enum_t<T>> = true;
template <typename T>
constexpr inline bool is_concrete_v<struct_t<T>> = true;
template <typename T>
constexpr inline bool is_concrete_v<union_t<T>> = true;
template <typename T>
constexpr inline bool is_concrete_v<exception_t<T>> = true;

template <typename ValTag, template <typename...> typename ListT>
constexpr inline bool is_concrete_v<list<ValTag, ListT>> =
    is_concrete_v<ValTag>;

template <typename KeyTag, template <typename...> typename SetT>
constexpr inline bool is_concrete_v<set<KeyTag, SetT>> = is_concrete_v<KeyTag>;
template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
constexpr inline bool is_concrete_v<map<KeyTag, ValTag, MapT>> =
    is_concrete_v<KeyTag>&& is_concrete_v<ValTag>;

template <typename Adapter, typename Tag>
constexpr inline bool is_concrete_v<adapted<Adapter, Tag>> = is_concrete_v<Tag>;
template <typename T, typename Tag>
constexpr inline bool is_concrete_v<cpp_type<T, Tag>> = is_concrete_v<Tag>;

template <>
constexpr inline bool is_thrift_type_tag_v<all_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<number_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<integral_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<floating_point_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<enum_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<string_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<structured_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<struct_except_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<struct_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<union_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<exception_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<container_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<list_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<set_c> = true;
template <>
constexpr inline bool is_thrift_type_tag_v<map_c> = true;

template <typename ValTag, template <typename...> typename ListT>
constexpr inline bool is_thrift_type_tag_v<list<ValTag, ListT>> =
    is_thrift_type_tag_v<ValTag>;
template <typename KeyTag, template <typename...> typename SetT>
constexpr inline bool is_thrift_type_tag_v<set<KeyTag, SetT>> =
    is_thrift_type_tag_v<KeyTag>;
template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
constexpr inline bool is_thrift_type_tag_v<map<KeyTag, ValTag, MapT>> =
    is_thrift_type_tag_v<KeyTag>&& is_thrift_type_tag_v<ValTag>;

template <typename Adapter, typename Tag>
constexpr inline bool is_thrift_type_tag_v<adapted<Adapter, Tag>> =
    is_thrift_type_tag_v<Tag>;
template <typename T, typename Tag>
constexpr inline bool is_thrift_type_tag_v<cpp_type<T, Tag>> =
    is_thrift_type_tag_v<Tag>;

template <typename V1, typename V2>
constexpr inline bool is_a_v<list<V1>, list<V2>> = is_a_v<V1, V2>;
template <typename K1, typename K2>
constexpr inline bool is_a_v<set<K1>, set<K2>> = is_a_v<K1, K2>;

template <typename K1, typename V1, typename K2, typename V2>
constexpr inline bool is_a_v<map<K1, V1>, map<K2, V2>> =
    is_a_v<K1, K2>&& is_a_v<V1, V2>;

template <typename A, typename Tag, typename CTag>
constexpr inline bool is_a_v<adapted<A, Tag>, CTag> = is_a_v<Tag, CTag>;
template <typename A, typename Tag, typename CTag>
constexpr inline bool is_a_v<adapted<A, Tag>, adapted<A, CTag>> =
    is_a_v<Tag, CTag>;

template <typename T, typename Tag, typename CTag>
constexpr inline bool is_a_v<cpp_type<T, Tag>, CTag> = is_a_v<Tag, CTag>;
template <typename T, typename Tag, typename CTag>
constexpr inline bool is_a_v<cpp_type<T, Tag>, cpp_type<T, CTag>> =
    is_a_v<Tag, CTag>;

} // namespace apache::thrift::type
