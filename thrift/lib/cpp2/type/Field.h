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

// The type tag for the given type::field_t.
template <typename FieldTag>
using field_type_tag = typename detail::field_to_tag::apply<FieldTag>::type;

// The FieldId for the given type::field_t
template <typename FieldTag>
FOLLY_INLINE_VARIABLE constexpr FieldId field_id_v =
    FieldId(detail::field_to_id::apply<FieldTag>::value);

} // namespace type
namespace field {
template <class Tag, class T>
using ordinal = typename detail::OrdinalImpl<Tag, T>::type;

template <class Tag, class T>
using id = ::apache::thrift::detail::st::struct_private_access::
    field_id<type::native_type<Tag>, ordinal<Tag, T>>;

template <class Tag, class T>
using type_tag = ::apache::thrift::detail::st::struct_private_access::
    type_tag<type::native_type<Tag>, ordinal<Tag, T>>;

template <class Tag, class T>
using ident = ::apache::thrift::detail::st::struct_private_access::
    ident<type::native_type<Tag>, ordinal<Tag, T>>;
} // namespace field
} // namespace thrift
} // namespace apache
