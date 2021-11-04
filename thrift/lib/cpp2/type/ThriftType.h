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

// Type tags for describing the 'shape' of thrift types at compile-time.
//
// _t indicates a concrete type.
// _c indicates a class of types.
// no suffix means it is dependent on the parameters.
//
// For example, `type::list_c` represents a list of any type,
// `type::list<type::enum_c>` represents a list of any enum type, and
// `type::list<type::enum_t<MyEnum>>` represents a list of MyEnums.
namespace apache::thrift::type {

// Type tags for all t types.
struct bool_t {};
struct byte_t {};
struct i16_t {};
struct i32_t {};
struct i64_t {};
struct float_t {};
struct double_t {};
struct string_t {};
struct binary_t {};

// The enum class of types.
struct enum_c {};
// A concrete enum type.
template <typename T>
struct enum_t {};

// The struct class of types.
struct struct_c {};
// A concrete struct type.
template <typename T>
struct struct_t {};

// The union class of types.
struct union_c {};
template <typename T>
struct union_t {};

// The exception class of types.
struct exception_c {};
template <typename T>
struct exception_t {};

// The list class of types.
struct list_c {};
template <typename VT>
struct list {};

// The set class of types.
struct set_c {};
template <typename VT>
struct set {};

// The map class of types.
struct map_c {};
template <typename KT, typename VT>
struct map {};

} // namespace apache::thrift::type
