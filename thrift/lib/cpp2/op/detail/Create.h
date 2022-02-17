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
#include <thrift/lib/cpp2/Adapt.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

template <typename Tag>
struct Create {
  static_assert(type::is_concrete_v<Tag>);

  template <typename T = type::native_type<Tag>>
  constexpr T operator()() const {
    return T{};
  }
};

template <typename Tag, FieldId Id>
struct Create<type::field_t<Id, Tag>> {
  static_assert(type::is_concrete_v<Tag>);

  template <typename Struct, typename T = type::native_type<Tag>>
  constexpr T operator()(Struct&) const {
    return T{};
  }
};

template <typename Adapter, typename Tag, FieldId Id>
struct Create<type::field_t<Id, type::adapted<Adapter, Tag>>> {
  using adapted_tag = type::adapted<Adapter, Tag>;
  static_assert(type::is_concrete_v<adapted_tag>);

  template <typename Struct, typename AdapterT = Adapter>
  constexpr adapt_detail::
      if_not_field_adapter<AdapterT, type::standard_type<Tag>, Struct>
      operator()(Struct&) const {
    return type::native_type<adapted_tag>{};
  }

  template <typename Struct, typename AdapterT = Adapter>
  constexpr adapt_detail::FromThriftFieldIdType<
      AdapterT,
      static_cast<int16_t>(Id),
      type::standard_type<Tag>,
      Struct>
  operator()(Struct& object) const {
    adapt_detail::FromThriftFieldIdType<
        AdapterT,
        static_cast<int16_t>(Id),
        type::standard_type<Tag>,
        Struct>
        obj{};
    adapt_detail::construct<Adapter, static_cast<int16_t>(Id)>(obj, object);
    return obj;
  }
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
