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
#include <thrift/lib/cpp/FieldId.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Tag.h>

namespace apache {
namespace thrift {
namespace type {
namespace detail {

template <typename Fields>
struct fields_size;
template <template <typename...> class F, typename... Fs>
struct fields_size<F<Fs...>> {
  static constexpr auto value = sizeof...(Fs);
};
template <typename Ts, std::size_t I, bool Valid = true>
struct field_at {
  using type = void;
};
template <typename... Fs, std::size_t I>
struct field_at<fields<Fs...>, I, true> {
  using type = folly::type_pack_element_t<I, Fs...>;
};
template <typename Ts, std::size_t I>
using field_at_t = typename field_at<Ts, I, (I < fields_size<Ts>::value)>::type;

template <typename StructTag>
using struct_fields =
    ::apache::thrift::detail::st::struct_private_access::fields<
        native_type<StructTag>>;

template <typename StructTag>
FOLLY_INLINE_VARIABLE constexpr std::size_t field_size_v =
    ::apache::thrift::detail::st::struct_private_access::
        __fbthrift_field_size_v<native_type<StructTag>>;

template <typename StructTag, Ordinal ord>
using if_valid_ordinal = std::enable_if_t<
    ord != Ordinal{0} && folly::to_underlying(ord) <= field_size_v<StructTag>>;

struct field_to_id {
  template <class>
  struct apply;
  template <FieldId Id, class Tag>
  struct apply<field_t<Id, Tag>> {
    static constexpr auto value = static_cast<field_id_u_t>(Id);
  };
};

struct field_to_tag {
  template <class>
  struct apply;
  template <FieldId Id, class Tag>
  struct apply<field_t<Id, Tag>> {
    using type = Tag;
  };
};

template <class StructTag, Ordinal ord>
struct field_tag_by_ord {
  using type = field_at_t<
      struct_fields<StructTag>,
      ord == Ordinal{0} ? static_cast<std::size_t>(-1)
                        : folly::to_underlying(ord) - 1>;
};

} // namespace detail
} // namespace type
} // namespace thrift
} // namespace apache
