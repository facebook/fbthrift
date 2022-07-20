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

#include <thrift/lib/cpp/Field.h>
#include <thrift/lib/cpp2/type/detail/Field.h>

namespace apache {
namespace thrift {
namespace type {

using detail::field_size_v;

template <class Tag, class T>
using get_field_ordinal = typename detail::OrdinalImpl<Tag, T>::type;

template <class Tag, class T>
using get_field_id = ::apache::thrift::detail::st::struct_private_access::
    field_id<type::native_type<Tag>, get_field_ordinal<Tag, T>>;

template <class Tag, class T>
using get_field_type_tag = ::apache::thrift::detail::st::struct_private_access::
    type_tag<type::native_type<Tag>, get_field_ordinal<Tag, T>>;

template <class Tag, class T>
using get_field_ident = ::apache::thrift::detail::st::struct_private_access::
    ident<type::native_type<Tag>, get_field_ordinal<Tag, T>>;

namespace detail {
template <class Tag, class T>
FOLLY_INLINE_VARIABLE constexpr bool exists =
    get_field_ordinal<Tag, T>::value != static_cast<FieldOrdinal>(0);

struct FieldTagImpl {
 public:
  template <class Tag, class T>
  using apply = type::field<
      get_field_type_tag<Tag, T>,
      FieldContext<
          type::native_type<Tag>,
          folly::to_underlying(get_field_id<Tag, T>::value)>>;
};
} // namespace detail

template <class Tag, class T>
using get_field_tag = typename std::conditional_t<
    detail::exists<Tag, T>,
    detail::FieldTagImpl,
    detail::MakeVoid>::template apply<Tag, T>;

template <class Tag, class T>
FOLLY_INLINE_VARIABLE constexpr FieldOrdinal get_field_ordinal_v =
    get_field_ordinal<Tag, T>::value;

template <class Tag, class T>
FOLLY_INLINE_VARIABLE constexpr FieldId get_field_id_v =
    get_field_id<Tag, T>::value;

// TODO(ytj): use a more specific template parameter name instead of `T`.
// Since we only accept type tag, id, ordinal or ident
template <class Tag, class T>
using get_field_native_type = native_type<get_field_tag<Tag, T>>;

// It calls the given function with field_ordinal<1> to field_ordinal<N>.
template <typename StructTag, typename F>
void for_each_ordinal(F f) {
  detail::for_each_ordinal_impl(
      f, std::make_integer_sequence<size_t, type::field_size_v<StructTag>>{});
}

} // namespace type
} // namespace thrift
} // namespace apache
