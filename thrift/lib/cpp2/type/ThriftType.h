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
struct is_concrete : std::false_type {};
template <typename Tag>
constexpr bool is_concrete_v = is_concrete<Tag>::value;
namespace bound {
struct is_concrete {
  template <typename Tag>
  using apply = type::is_concrete<Tag>;
};
} // namespace bound

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
struct is_thrift_type_tag : is_concrete<Tag> {};
template <typename Tag>
constexpr bool is_thrift_type_tag_v = is_thrift_type_tag<Tag>::value;
namespace bound {
struct is_thrift_type_tag {
  template <typename Tag>
  using apply = type::is_thrift_type_tag<Tag>;
};
} // namespace bound

// If a given Thrift type tag is not concrete.
//
// For example:
//     is_not_concrete_v<int> -> false
//     is_not_concrete_v<byte_t> -> false
//     is_not_concrete_v<list_c> -> true
//     is_not_concrete_v<list<Foo>> -> false
//     is_not_concrete_v<list<byte_t>> -> false
//     is_not_concrete_v<list<struct_c>> -> true
template <typename Tag>
constexpr bool is_not_concrete_v =
    is_thrift_type_tag_v<Tag> && !is_concrete_v<Tag>;
template <typename Tag>
using is_not_concrete = std::bool_constant<is_not_concrete_v<Tag>>;
namespace bound {
struct is_not_concrete {
  template <typename Tag>
  using apply = type::is_not_concrete<Tag>;
};
} // namespace bound

// Helpers to enable/disable declarations based on if a type tag represents
// a concrete type or not.
template <typename Tag, typename R = void, typename...>
using if_concrete = std::enable_if_t<is_concrete_v<Tag>, R>;
template <typename Tag, typename R = void, typename...>
using if_not_concrete = std::enable_if_t<is_not_concrete_v<Tag>, R>;

////
// Implemnation details

template <>
struct is_concrete<void_t> : std::true_type {};
template <>
struct is_concrete<bool_t> : std::true_type {};
template <>
struct is_concrete<byte_t> : std::true_type {};
template <>
struct is_concrete<i16_t> : std::true_type {};
template <>
struct is_concrete<i32_t> : std::true_type {};
template <>
struct is_concrete<i64_t> : std::true_type {};
template <>
struct is_concrete<float_t> : std::true_type {};
template <>
struct is_concrete<double_t> : std::true_type {};
template <>
struct is_concrete<string_t> : std::true_type {};
template <>
struct is_concrete<binary_t> : std::true_type {};

template <typename T>
struct is_concrete<enum_t<T>> : std::true_type {};
template <typename T>
struct is_concrete<struct_t<T>> : std::true_type {};
template <typename T>
struct is_concrete<union_t<T>> : std::true_type {};
template <typename T>
struct is_concrete<exception_t<T>> : std::true_type {};

template <typename ValTag, template <typename...> typename ListT>
struct is_concrete<list<ValTag, ListT>> : is_concrete<ValTag> {};

template <typename KeyTag, template <typename...> typename SetT>
struct is_concrete<set<KeyTag, SetT>> : is_concrete<KeyTag> {};
template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
struct is_concrete<map<KeyTag, ValTag, MapT>>
    : std::bool_constant<is_concrete_v<KeyTag> && is_concrete_v<ValTag>> {};

template <typename Adapter, typename Tag>
struct is_concrete<adapted<Adapter, Tag>> : is_concrete<Tag> {};
template <typename T, typename Tag>
struct is_concrete<cpp_type<T, Tag>> : is_concrete<Tag> {};

template <>
struct is_thrift_type_tag<integral_c> : std::true_type {};
template <>
struct is_thrift_type_tag<floating_point_c> : std::true_type {};
template <>
struct is_thrift_type_tag<enum_c> : std::true_type {};
template <>
struct is_thrift_type_tag<struct_except_c> : std::true_type {};
template <>
struct is_thrift_type_tag<struct_c> : std::true_type {};
template <>
struct is_thrift_type_tag<union_c> : std::true_type {};
template <>
struct is_thrift_type_tag<exception_c> : std::true_type {};
template <>
struct is_thrift_type_tag<list_c> : std::true_type {};
template <>
struct is_thrift_type_tag<set_c> : std::true_type {};
template <>
struct is_thrift_type_tag<map_c> : std::true_type {};

template <typename ValTag, template <typename...> typename ListT>
struct is_thrift_type_tag<list<ValTag, ListT>> : is_thrift_type_tag<ValTag> {};
template <typename KeyTag, template <typename...> typename SetT>
struct is_thrift_type_tag<set<KeyTag, SetT>> : is_thrift_type_tag<KeyTag> {};
template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
struct is_thrift_type_tag<map<KeyTag, ValTag, MapT>>
    : std::bool_constant<
          is_thrift_type_tag_v<KeyTag> && is_thrift_type_tag_v<ValTag>> {};

template <typename Adapter, typename Tag>
struct is_thrift_type_tag<adapted<Adapter, Tag>> : is_thrift_type_tag<Tag> {};
template <typename T, typename Tag>
struct is_thrift_type_tag<cpp_type<T, Tag>> : is_thrift_type_tag<Tag> {};

} // namespace apache::thrift::type
