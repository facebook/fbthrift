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

#include <cmath>

#include <folly/Overload.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/op/Compare.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/ThriftType.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

// C++'s intrinsic default for the underlying native type, is the intrisitic
// default for for all unstructured types.
// TODO(dokwon): Support field_ref types.
template <typename T>
constexpr T getIntrinsicDefault(type::all_c) {
  return T{};
}

template <typename T>
constexpr T getIntrinsicDefault(type::string_c) {
  return StringTraits<T>::fromStringLiteral("");
}

template <typename T>
FOLLY_EXPORT const T& getIntrinsicDefault(type::structured_c) noexcept {
  const static T* kDefault = []() {
    auto* value = new T{};
    // The default construct respects 'custom' defaults on fields, but
    // clearing any instance of a structured type, sets it to the
    // 'intrinsic' default.
    apache::thrift::clear(*value);
    return value;
  }();
  return *kDefault;
}

template <typename Tag>
struct Clear {
  static_assert(type::is_concrete_v<Tag>, "");
  template <typename T>
  void operator()(T& value) const {
    folly::overload(
        [](auto& v, type::structured_c) { apache::thrift::clear(v); },
        [](auto& v, type::container_c) { v.clear(); },
        [](auto& v, type::all_c) {
          // All unstructured types can be cleared by assigning to the intrinsic
          // default.
          v = getIntrinsicDefault<T>(Tag{});
        })(value, Tag{});
  }
};

template <typename Adapter, typename Tag>
struct Clear<type::adapted<Adapter, Tag>> {
  using adapted_tag = type::adapted<Adapter, Tag>;
  static_assert(type::is_concrete_v<adapted_tag>, "");
  template <typename T>
  void operator()(T& value) const {
    ::apache::thrift::adapt_detail::clear<Adapter>(value);
  }
};

// TODO(dokwon): Support field_ref types.
template <typename Tag, typename Context>
struct Clear<type::field_tag<Tag, Context>> : Clear<Tag> {};

template <typename Adapter, typename Tag, typename Struct, int16_t FieldId>
struct Clear<type::field_tag<
    type::adapted<Adapter, Tag>,
    FieldContext<Struct, FieldId>>> {
  using field_adapted_tag = type::
      field_tag<type::adapted<Adapter, Tag>, FieldContext<Struct, FieldId>>;
  static_assert(type::is_concrete_v<field_adapted_tag>, "");
  template <typename T>
  void operator()(T& value, Struct& obj) const {
    ::apache::thrift::adapt_detail::clear<Adapter, FieldId>(value, obj);
  }
};

template <typename Tag>
struct Empty {
  static_assert(type::is_concrete_v<Tag>, "");
  template <typename T = type::native_type<Tag>>
  constexpr bool operator()(const T& value) const {
    return folly::overload(
        [](const auto& v, type::string_c) {
          return StringTraits<T>::isEmpty(v);
        },
        [](const auto& v, type::container_c) { return v.empty(); },
        [](const auto& v, type::structured_c) {
          return apache::thrift::empty(v);
        },
        [](const auto& v, type::all_c) {
          // All unstructured values are 'empty' if they are identical to their
          // intrinsic default.
          return op::identical<Tag>(v, getIntrinsicDefault<T>(Tag{}));
        })(value, Tag{});
  }
};

template <typename Adapter, typename Tag>
struct Empty<type::adapted<Adapter, Tag>> {
  using adapted_tag = type::adapted<Adapter, Tag>;
  static_assert(type::is_concrete_v<adapted_tag>, "");
  template <typename T>
  constexpr bool operator()(const T& value) const {
    return op::identical<adapted_tag>(
        value, getIntrinsicDefault<T>(adapted_tag{}));
  }
};

// TODO(dokwon): Support field_ref types.
template <typename Tag, typename Context>
struct Empty<type::field_tag<Tag, Context>> : Empty<Tag> {};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
