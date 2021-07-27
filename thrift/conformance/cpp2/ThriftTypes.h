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

// Utilities for describing the 'shape' of thrift types at compile-time.
//
// TODO(afuller): Simplify as much as possible, and merging into
// TypeClass.h. The classes defined here are a natural extension
// of the ones in TypeClass.h

#pragma once

#include <thrift/conformance/cpp2/internal/ThriftTypes.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fatal/type/cat.h>
#include <folly/Range.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp/protocol/TType.h>

namespace apache::thrift::conformance {

// _t indicates a concrete type.
// _c indicates a class of types.
// no sufix means it is dependent on the parameters.
namespace type {

// Type tags for all primitive types.
using bool_t = detail::primitive_type<BaseType::Bool, bool>;
using byte_t = detail::primitive_type<BaseType::Byte, int8_t, uint8_t>;
using i16_t = detail::primitive_type<BaseType::I16, int16_t, uint16_t>;
using i32_t = detail::primitive_type<BaseType::I32, int32_t, uint32_t>;
using i64_t = detail::primitive_type<BaseType::I64, int64_t, uint64_t>;
using float_t = detail::primitive_type<BaseType::Float, float>;
using double_t = detail::primitive_type<BaseType::Double, double>;
using string_t = detail::primitive_type<
    BaseType::String,
    std::string,
    std::string_view,
    folly::StringPiece>;
using binary_t = detail::primitive_type<
    BaseType::Binary,
    std::string,
    folly::IOBuf,
    folly::ByteRange>;

// The enum class of types.
using enum_c = detail::base_type<BaseType::Enum>;
template <typename E>
using enum_t = detail::cpp_type<BaseType::Enum, E>;

// The struct class of types.
using struct_c = detail::base_type<BaseType::Struct>;
template <typename T>
using struct_t = detail::cpp_type<BaseType::Struct, T>;

// The union class of types.
using union_c = detail::base_type<BaseType::Union>;
template <typename T>
using union_t = detail::cpp_type<BaseType::Union, T>;

// The exception class of types.
using exception_c = detail::base_type<BaseType::Exception>;
template <typename T>
using exception_t = detail::cpp_type<BaseType::Exception, T>;

// The list class of types.
using list_c = detail::base_type<BaseType::List>;
template <typename VT>
using list = detail::list<VT>;

// The set class of types.
using set_c = detail::base_type<BaseType::Set>;
template <typename VT>
using set = detail::set<VT>;

// The map class of types.
using map_c = detail::base_type<BaseType::Map>;
template <typename KT, typename VT>
using map = detail::map<KT, VT>;

} // namespace type

// If a given type tag refers to concrete type and not a class of types.
// For example:
//     is_concrete_type_v<type::byte_t> -> true
//     is_concrete_type_v<type::list_c> -> false
//     is_concrete_type_v<type::list<type::byte_t>> -> true
//     is_concrete_type_v<type::list<type::struct_c>> -> false
template <typename T>
using is_concrete_type = detail::is_concrete_type<T>;
template <typename T>
inline constexpr bool is_concrete_type_v = is_concrete_type<T>::value;
template <typename T, typename R = void>
using if_concrete = std::enable_if<is_concrete_type_v<T>, R>;

namespace bound {
struct is_concrete_type {
  template <typename T>
  using apply = detail::is_concrete_type<T>;
};
} // namespace bound

// Useful groupings of primitive types.
using integral_types = detail::types<
    type::bool_t,
    type::byte_t,
    type::i16_t,
    type::i32_t,
    type::i64_t,
    type::enum_c>;
using floating_point_types = detail::types<type::float_t, type::double_t>;
using numeric_types = fatal::cat<integral_types, floating_point_types>;
using string_types = detail::types<type::string_t, type::binary_t>;

// All primitive types.
using primitive_types = fatal::cat<numeric_types, string_types>;

// All structured types.
using structured_types =
    detail::types<type::struct_c, type::union_c, type::exception_c>;

// Types that are a single value.
using singular_types = fatal::cat<primitive_types, structured_types>;
// Types that are containers of other types.
using container_types = detail::types<type::list_c, type::set_c, type::map_c>;
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

} // namespace apache::thrift::conformance
