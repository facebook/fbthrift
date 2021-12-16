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

namespace detail {
// A place holder for the default template for a container.
template <typename...>
struct DefaultT;
} // namespace detail

// Classes of types (_c suffix).
struct enum_c {}; // all `enum` types
struct struct_c {}; // all `struct` types
struct union_c {}; // all `union` types
struct exception_c {}; // all `exception` types
struct list_c {}; // all `list`  types
struct set_c {}; // all `set` types
struct map_c {}; // all `map` types

// Type tags for types that are always concrete (_t suffix).
struct void_t {};
struct bool_t {};
struct byte_t {};
struct i16_t {};
struct i32_t {};
struct i64_t {};
struct float_t {};
struct double_t {};
struct string_t {};
struct binary_t {};
template <typename T> // the generated cpp type
struct enum_t : enum_c {};
template <typename T> // the generated cpp type
struct struct_t : struct_c {};
template <typename T> // the generated cpp type
struct union_t : union_c {};
template <typename T> // the generated cpp type
struct exception_t : exception_c {};

// Parameterized types.
template <
    typename ValTag,
    // the cpp template type
    template <typename...> typename ListT = detail::DefaultT>
struct list : list_c {};
template <
    typename KeyTag,
    // the cpp template type
    template <typename...> typename SetT = detail::DefaultT>
struct set : set_c {};
template <
    typename KeyTag,
    typename ValTag,
    // the cpp template type
    template <typename...> typename MapT = detail::DefaultT>
struct map : map_c {};

// An adapted type.
template <
    typename Adapter, // the cpp adpter to use
    typename Tag> // the thrift type being adapted
struct adapted : Tag {};

// A type mapped to a specific c++ type.
//
// The given type must be a 'drop in' replacement for the
// native_type it is overriding.
template <
    typename T, // the cpp type to use
    typename Tag> // the thrift type being overridden
struct cpp_type : Tag {};

} // namespace apache::thrift::type
