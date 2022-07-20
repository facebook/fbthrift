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

#include <folly/Traits.h>
#include <folly/Utility.h>
#include <thrift/lib/cpp/Field.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Tag.h>

namespace apache {
namespace thrift {
namespace type {
namespace detail {

template <typename StructTag>
FOLLY_INLINE_VARIABLE constexpr std::size_t field_size_v =
    ::apache::thrift::detail::st::struct_private_access::
        __fbthrift_field_size_v<native_type<StructTag>>;

template <class T>
FOLLY_INLINE_VARIABLE constexpr bool is_field_id_v = false;

template <FieldId Id>
FOLLY_INLINE_VARIABLE constexpr bool
    is_field_id_v<std::integral_constant<FieldId, Id>> = true;

template <class T>
FOLLY_INLINE_VARIABLE constexpr bool is_field_ordinal_v = false;

template <FieldOrdinal Ordinal>
FOLLY_INLINE_VARIABLE constexpr bool
    is_field_ordinal_v<std::integral_constant<FieldOrdinal, Ordinal>> = true;

template <class T, class = void>
FOLLY_INLINE_VARIABLE constexpr bool is_field_ident_v = false;

template <class T>
FOLLY_INLINE_VARIABLE constexpr bool is_field_ident_v<
    T,
    decltype(__fbthrift_check_whether_type_is_ident_via_adl(
        FOLLY_DECLVAL(T &&)))> = true;

void is_type_tag_impl(all_c);
void is_type_tag_impl(void_t);
void is_type_tag_impl(service_c);

// We can't use std::is_base_of since it's undefined behavior to use it on
// incomplete type, and ident is incomplete type.
template <class T, class = void>
FOLLY_INLINE_VARIABLE constexpr bool is_type_tag_v = false;
template <class T>
FOLLY_INLINE_VARIABLE constexpr bool
    is_type_tag_v<T, decltype(is_type_tag_impl(FOLLY_DECLVAL(T &&)))> = true;

// TODO(ytj): Disallow void type
template <class T>
FOLLY_INLINE_VARIABLE constexpr bool is_reflection_metadata_v =
    is_type_tag_v<T> || is_field_id_v<T> || is_field_ordinal_v<T> ||
    is_field_ident_v<T> || std::is_void<T>::value;

template <class Tag, class T>
struct OrdinalImpl {
  // TODO(ytj): To reduce build time, only check whether T is reflection
  // metadata if we couldn't find T.
  static_assert(is_reflection_metadata_v<T>, "");
  using type = ::apache::thrift::detail::st::struct_private_access::
      ordinal<type::native_type<Tag>, T>;
};

template <class Tag, FieldOrdinal Ord>
struct OrdinalImpl<Tag, std::integral_constant<FieldOrdinal, Ord>> {
  static_assert(
      folly::to_underlying(Ord) <=
          ::apache::thrift::detail::st::struct_private_access::
              __fbthrift_field_size_v<type::native_type<Tag>>,
      "FieldOrdinal cannot be larger than the number of fields");

  // T is an ordinal, return itself
  using type = std::integral_constant<FieldOrdinal, Ord>;
};

template <class Tag, class TypeTag, class Struct, int16_t Id>
struct OrdinalImpl<Tag, type::field<TypeTag, FieldContext<Struct, Id>>>
    : OrdinalImpl<Tag, field_id<Id>> {};

struct MakeVoid {
  template <class...>
  using apply = void;
};

template <size_t... I, typename F>
void for_each_ordinal_impl(F f, std::index_sequence<I...>) {
  // This doesn't use fold expression (from C++17) as this file is used in
  // C++14 environment as well.
  auto unused = {(f(field_ordinal<I + 1>()), 0)...};
  static_cast<void>(unused);
}

} // namespace detail
} // namespace type
} // namespace thrift
} // namespace apache
