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

#include <thrift/lib/cpp/FieldId.h>
#include <thrift/lib/cpp2/type/detail/Field.h>

namespace apache {
namespace thrift {
namespace type {

using detail::field_size_v;

template <class StructTag>
using ordinal_fn =
    ::apache::thrift::detail::st::struct_private_access::ordinal_fn<
        native_type<StructTag>>;

template <class StructTag>
FOLLY_INLINE_VARIABLE constexpr ordinal_fn<StructTag> ordinal{};

// The type tag for the given type::field_t.
template <typename FieldTag>
using field_type_tag = typename detail::field_to_tag::apply<FieldTag>::type;

// The FieldId for the given type::field_t
template <typename FieldTag>
FOLLY_INLINE_VARIABLE constexpr FieldId field_id_v =
    FieldId(detail::field_to_id::apply<FieldTag>::value);

template <typename StructTag, Ordinal ord>
using field_tag_by_ordinal =
    typename detail::field_tag_by_ord<StructTag, ord>::type;

template <
    typename StructTag,
    Ordinal ord,
    detail::if_valid_ordinal<StructTag, ord>* = nullptr>
FOLLY_INLINE_VARIABLE constexpr FieldId field_id_by_ordinal_v =
    ::apache::thrift::detail::st::struct_private_access::__fbthrift_field_ids<
        native_type<StructTag>>[folly::to_underlying(ord) - 1];

template <typename Structured, Ordinal ord>
using field_type_tag_by_ordinal =
    field_type_tag<field_tag_by_ordinal<Structured, ord>>;

// field_tag_t<StructTag, Id> is an alias for the field_t of the field in given
// thrift struct where corresponding field id == Id, or for void if there is no
// such field.
template <typename StructTag, FieldId Id>
using field_tag_by_id = field_tag_by_ordinal<StructTag, ordinal<StructTag>(Id)>;

} // namespace type
} // namespace thrift
} // namespace apache
