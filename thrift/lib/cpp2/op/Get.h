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
#include <thrift/lib/cpp/Field.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/type/Id.h>
#include <thrift/lib/cpp2/type/NativeType.h>

namespace apache {
namespace thrift {
namespace op {

// TODO(afuller): Consider moving to a new detail/Get.h as these are going to
// get larger.
namespace detail {
template <class Tag, class Id>
struct GetOrdinalImpl;
template <size_t... I, typename F>
void for_each_ordinal_impl(F&& f, std::index_sequence<I...>);
} // namespace detail

// Resolves to the number of definitions contained in Tag.
template <typename Tag>
FOLLY_INLINE_VARIABLE constexpr std::size_t size_v = ::apache::thrift::detail::
    st::struct_private_access::__fbthrift_field_size_v<type::native_type<Tag>>;

// Gets the ordinal, for example:
//
//   // Resolves to ordinal at which the field "foo" was defined in MyStruct.
//   using Ord = get_ordinal<struct_t<MyStruct>, ident::foo>
//
template <class Tag, class Id>
using get_ordinal = typename detail::GetOrdinalImpl<Tag, Id>::type;
template <class Tag, class Id>
FOLLY_INLINE_VARIABLE constexpr type::Ordinal get_ordinal_v =
    get_ordinal<Tag, Id>::value;

// It calls the given function with ordinal<1> to ordinal<N>.
template <typename Tag, typename F>
void for_each_ordinal(F&& f) {
  detail::for_each_ordinal_impl(
      std::forward<F>(f), std::make_integer_sequence<size_t, size_v<Tag>>{});
}

// Gets the field id, for example:
//
//   // Resolves to field id assigned to the field "foo" in MyStruct.
//   using FieldId = get_field_id<struct_t<MyStruct>, ident::foo>
//
template <class Tag, class Id>
using get_field_id = folly::conditional_t<
    get_ordinal<Tag, Id>::value == type::Ordinal{},
    field_id<0>,
    ::apache::thrift::detail::st::struct_private_access::
        field_id<type::native_type<Tag>, get_ordinal<Tag, Id>>>;
template <class Tag, class Id>
FOLLY_INLINE_VARIABLE constexpr FieldId get_field_id_v =
    get_field_id<Tag, Id>::value;

// It calls the given function with each field_id<{id}> in Tag.
template <typename Tag, typename F>
void for_each_field_id(F&& f) {
  for_each_ordinal<Tag>(
      [&](auto ord) { f(get_field_id<Tag, decltype(ord)>{}); });
}

// Gets the ident, for example:
//
//   // Resolves to thrift::ident::* type associated with field 7 in MyStruct.
//   using Ident = get_field_id<struct_t<MyStruct>, field_id<7>>
//
template <class Tag, class Id>
using get_ident = ::apache::thrift::detail::st::struct_private_access::
    ident<type::native_type<Tag>, get_ordinal<Tag, Id>>;

// It calls the given function with each folly::tag<thrift::ident::*>{} in Tag.
template <typename Tag, typename F>
void for_each_ident(F&& f) {
  for_each_ordinal<Tag>(
      [&](auto ord) { f(folly::tag_t<get_ident<Tag, decltype(ord)>>{}); });
}

// Gets the Thrift type tag, for example:
//
//   // Resolves to Thrift type tag for the field "foo" in MyStruct.
//   using Tag = get_field_id<struct_t<MyStruct>, ident::foo>
//
template <typename Tag, typename Id>
using get_type_tag = ::apache::thrift::detail::st::struct_private_access::
    type_tag<type::native_type<Tag>, get_ordinal<Tag, Id>>;

template <class Tag, class Id>
using get_field_tag = typename std::conditional_t<
    get_ordinal<Tag, Id>::value == type::Ordinal{},
    void,
    type::field<
        get_type_tag<Tag, Id>,
        FieldContext<
            type::native_type<Tag>,
            folly::to_underlying(get_field_id<Tag, Id>::value)>>>;

template <class Tag, class Id>
using get_native_type = type::native_type<get_field_tag<Tag, Id>>;

template <typename Tag, class Id>
FOLLY_INLINE_VARIABLE constexpr auto get = access_field<get_ident<Tag, Id>>;

namespace detail {
template <class Tag, class Id>
struct GetOrdinalImpl {
  // TODO(ytj): To reduce build time, only check whether Id is reflection
  // metadata if we couldn't find Id.
  static_assert(type::is_id_v<Id>, "");
  using type = ::apache::thrift::detail::st::struct_private_access::
      ordinal<type::native_type<Tag>, Id>;
};

template <class Tag, type::Ordinal Ord>
struct GetOrdinalImpl<Tag, std::integral_constant<type::Ordinal, Ord>> {
  static_assert(
      folly::to_underlying(Ord) <= size_v<Tag>,
      "Ordinal cannot be larger than the number of definitions");

  // Id is an ordinal, return itself
  using type = type::ordinal_tag<Ord>;
};

template <class Tag, class TypeTag, class Struct, int16_t Id>
struct GetOrdinalImpl<Tag, type::field<TypeTag, FieldContext<Struct, Id>>>
    : GetOrdinalImpl<Tag, field_id<Id>> {};

template <size_t... I, typename F>
void for_each_ordinal_impl(F&& f, std::index_sequence<I...>) {
  // This doesn't use fold expression (from C++17) as this file is used in
  // C++14 environment as well.
  int unused[] = {(f(field_ordinal<I + 1>()), 0)...};
  static_cast<void>(unused);
}

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
