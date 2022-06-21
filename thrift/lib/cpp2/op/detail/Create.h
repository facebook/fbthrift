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
#include <thrift/lib/cpp2/Adapt.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/ThriftType.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

template <typename Tag>
struct Create {
  static_assert(type::is_concrete_v<Tag>, "");

  template <typename T = type::native_type<Tag>>
  constexpr T operator()() const {
    return T{};
  }
};

// TODO(dokwon): Support field_ref types.
template <typename Tag, typename Context>
struct Create<type::field<Tag, Context>> : Create<Tag> {};

template <typename Adapter, typename Tag, typename Struct, int16_t FieldId>
struct Create<
    type::field<type::adapted<Adapter, Tag>, FieldContext<Struct, FieldId>>> {
  using field_adapted_tag =
      type::field<type::adapted<Adapter, Tag>, FieldContext<Struct, FieldId>>;
  static_assert(type::is_concrete_v<field_adapted_tag>, "");

  template <typename T = type::native_type<field_adapted_tag>>
  constexpr T operator()(Struct& object) const {
    T obj{};
    adapt_detail::construct<Adapter, FieldId>(obj, object);
    return obj;
  }
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
