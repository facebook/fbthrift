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

#pragma once

#include <fatal/type/cat.h>
#include <fatal/type/find.h>
#include <fatal/type/list.h>

namespace apache::thrift::conformance {

// A list of types with a few relevant helper memebers.
template <typename... Ts>
struct types {
  template <typename T>
  static constexpr bool contains = fatal::contains<types, T>::value;

  // Converts the type list to a type list of the given types.
  template <template <typename...> typename T>
  using as = T<Ts...>;
};

// An enumeration of all base types in thrift.
//
// Base types are unparametarized. For example, the base
// type of map<int, string> is BaseType::Map and the base type of
// all thrift structs is BaseType::Struct.
//
// Similar to lib/cpp/protocol/TType.h, but IDL
// concepts instead of protocol concepts.
enum class BaseType {
  // Integral primitive types.
  Bool,
  Byte,
  I16,
  I32,
  I64,

  // Floating point primitive types.
  Float,
  Double,

  // Enum type classes.
  Enum,

  // String primitive types.
  String,
  Binary,

  // Structured type classes.
  Struct,
  Union,
  Exception,

  // Container type classes.
  List,
  Set,
  Map,
};

// A tag class for the given base type.
template <BaseType B>
struct base_type {
  static constexpr BaseType type = B;
};

namespace type {

// Type tags for all primitive types.
using bool_t = base_type<BaseType::Bool>;
using byte_t = base_type<BaseType::Byte>;
using i16_t = base_type<BaseType::I16>;
using i32_t = base_type<BaseType::I32>;
using i64_t = base_type<BaseType::I64>;
using enum_t = base_type<BaseType::Enum>;
using float_t = base_type<BaseType::Float>;
using double_t = base_type<BaseType::Double>;
using string_t = base_type<BaseType::String>;
using binary_t = base_type<BaseType::Binary>;

// Type tags for classes of structored types.
using struct_t = base_type<BaseType::Struct>;
using union_t = base_type<BaseType::Union>;
using exception_t = base_type<BaseType::Exception>;

// Type tags for classes of container types.
using list_t = base_type<BaseType::List>;
using set_t = base_type<BaseType::Set>;
using map_t = base_type<BaseType::Map>;

// Type tags for parameterized types.
template <typename V>
struct list : list_t {
  using value_type = V;
};

template <typename V>
struct set : set_t {
  using value_type = V;
};

template <typename K, typename V>
struct map : map_t {
  using key_type = K;
  using mapped_type = V;
};

} // namespace type

// Useful groupings of primitive types.
using integral_types = types<
    type::bool_t,
    type::byte_t,
    type::i16_t,
    type::i32_t,
    type::i64_t,
    type::enum_t>;
using floating_point_types = types<type::float_t, type::double_t>;
using numeric_types = fatal::cat<integral_types, floating_point_types>;
using string_types = types<type::string_t, type::binary_t>;

// All primitive types.
using primitive_types = fatal::cat<numeric_types, string_types>;

// All structured types.
using structured_types =
    types<type::struct_t, type::union_t, type::exception_t>;

// Types that are a single value.
using singular_types = fatal::cat<primitive_types, structured_types>;
// Types that are containers of other types.
using container_types = types<type::list_t, type::set_t, type::map_t>;
// Types that are composites of other types.
using composite_types = fatal::cat<structured_types, container_types>;

} // namespace apache::thrift::conformance
