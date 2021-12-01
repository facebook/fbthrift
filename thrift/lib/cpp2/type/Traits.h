/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

// Utilities for working with ThriftType tags.
//
// TODO(afuller): Lots more docs (for now see tests for usage).
#pragma once

#include <thrift/lib/cpp2/type/detail/Traits.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fatal/type/cat.h>
#include <folly/Range.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp/protocol/TType.h>
#include <thrift/lib/thrift/gen-cpp2/type_types.h>

namespace apache::thrift::type {

// The BaseType for the given ThriftType.
template <typename Tag>
constexpr BaseType base_type_v = detail::traits<Tag>::kBaseType;

// The standard types associated with the given concrete ThriftType.
//
// These are the types that can be used directly with the Thrift
// runtime libraries.
//
// TODO(afuller): Consider removing this, as it is difficult/impossible to
// represent this as a list of types (something like c++20 concepts would be
// needed).
template <typename Tag>
using standard_types = typename detail::traits<Tag>::standard_types;

// The default standard type associated with the given concrete ThriftType.
//
// This is the type that is used by default, to represent the given ThriftType.
template <typename Tag>
using standard_type = typename detail::traits<Tag>::standard_type;

// The native type associated with the given concrete ThriftType.
//
// This is actual used by thrift to represent a value, taking
// into account any IDL annotations that modify the c++ type.
template <typename Tag>
using native_type = typename detail::traits<Tag>::native_type;

// If T is a standard type for the given ThriftType.
template <typename Tag, typename T>
struct is_standard_type : fatal::contains<standard_types<Tag>, T> {};

// Useful groupings of primitive types.
using integral_types =
    detail::types<bool_t, byte_t, i16_t, i32_t, i64_t, enum_c>;
using floating_point_types = detail::types<float_t, double_t>;
using numeric_types = fatal::cat<integral_types, floating_point_types>;
using string_types = detail::types<string_t, binary_t>;

// All primitive types.
using primitive_types = fatal::cat<numeric_types, string_types>;

// All structured types.
using structured_types = detail::types<struct_c, union_c, exception_c>;

// Types with an IDL specified name
using named_types = fatal::cat<structured_types, detail::types<enum_c>>;

// Types that are a single value.
using singular_types = fatal::cat<primitive_types, structured_types>;
// Types that are containers of other types.
using container_types = detail::types<list_c, set_c, map_c>;
// Types that are composites of other types.
using composite_types = fatal::cat<structured_types, container_types>;
// All types.
using all_types = fatal::cat<singular_types, container_types>;

// Types that are only defined if the given type belongs to the
// given group.
template <typename T, typename R = void>
using if_integral = detail::if_contains<integral_types, T, R>;
template <typename T, typename R = void>
using if_floating_point = detail::if_contains<floating_point_types, T, R>;
template <typename T, typename R = void>
using if_numeric = detail::if_contains<numeric_types, T, R>;
template <typename T, typename R = void>
using if_string = detail::if_contains<string_types, T, R>;
template <typename T, typename R = void>
using if_primitive = detail::if_contains<primitive_types, T, R>;
template <typename T, typename R = void>
using if_structured = detail::if_contains<structured_types, T, R>;
template <typename T, typename R = void>
using if_singular = detail::if_contains<singular_types, T, R>;
template <typename T, typename R = void>
using if_container = detail::if_contains<container_types, T, R>;
template <typename T, typename R = void>
using if_composite = detail::if_contains<composite_types, T, R>;

// Only defined if T has the BaseType B.
template <typename T, BaseType B, typename R = void>
using if_base_type = std::enable_if_t<B == T::kBaseType, R>;

constexpr inline TType toTType(BaseType type) {
  switch (type) {
    case BaseType::Void:
      return TType::T_VOID;
    case BaseType::Bool:
      return TType::T_BOOL;
    case BaseType::Byte:
      return TType::T_BYTE;
    case BaseType::I16:
      return TType::T_I16;
    case BaseType::Enum:
    case BaseType::I32:
      return TType::T_I32;
    case BaseType::I64:
      return TType::T_I64;
    case BaseType::Double:
      return TType::T_DOUBLE;
    case BaseType::Float:
      return TType::T_FLOAT;
    case BaseType::String:
      return TType::T_UTF8;
    case BaseType::Binary:
      return TType::T_STRING;

    case BaseType::List:
      return TType::T_LIST;
    case BaseType::Set:
      return TType::T_SET;
    case BaseType::Map:
      return TType::T_MAP;

    case BaseType::Struct:
      return TType::T_STRUCT;
    case BaseType::Union:
      return TType::T_STRUCT;
    case BaseType::Exception:
      return TType::T_STRUCT;
    default:
      folly::throw_exception<std::invalid_argument>(
          "Unsupported conversion from: " + std::to_string((int)type));
  }
}

constexpr inline BaseType toThriftBaseType(TType type) {
  switch (type) {
    case TType::T_BOOL:
      return BaseType::Bool;
    case TType::T_BYTE:
      return BaseType::Byte;
    case TType::T_I16:
      return BaseType::I16;
    case TType::T_I32:
      return BaseType::I32;
    case TType::T_I64:
      return BaseType::I64;
    case TType::T_DOUBLE:
      return BaseType::Double;
    case TType::T_FLOAT:
      return BaseType::Float;
    case TType::T_LIST:
      return BaseType::List;
    case TType::T_MAP:
      return BaseType::Map;
    case TType::T_SET:
      return BaseType::Set;
    case TType::T_STRING:
      return BaseType::Binary;
    case TType::T_STRUCT:
      return BaseType::Struct;
    case TType::T_UTF8:
      return BaseType::String;
    case TType::T_VOID:
      return BaseType::Void;
    default:
      folly::throw_exception<std::invalid_argument>(
          "Unsupported conversion from: " + std::to_string(type));
  }
}

} // namespace apache::thrift::type
