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

} // namespace detail
} // namespace type
namespace field {
namespace detail {
template <class Tag, class T>
struct OrdinalImpl {
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

} // namespace detail
} // namespace field
} // namespace thrift
} // namespace apache
