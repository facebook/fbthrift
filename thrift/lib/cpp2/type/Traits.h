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

// Utilities for working with ThriftType tags.
//
// TODO(afuller): Lots more docs (for now see tests for usage).
#pragma once

#include <fatal/type/cat.h>
#include <thrift/lib/cpp2/type/detail/Traits.h>

namespace apache::thrift::type {

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

// All primitive types.
using primitive_types = detail::types<
    bool_t,
    byte_t,
    i16_t,
    i32_t,
    i64_t,
    float_t,
    double_t,
    enum_c,
    string_t,
    binary_t>;

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

// Types that are only defined if the given type belongs to the
// given group.
template <typename T, typename R = void>
using if_primitive = detail::if_contains<primitive_types, T, R>;
template <typename T, typename R = void>
using if_structured = detail::if_contains<structured_types, T, R>;
template <typename T, typename R = void>
using if_named = detail::if_contains<named_types, T, R>;
template <typename T, typename R = void>
using if_singular = detail::if_contains<singular_types, T, R>;
template <typename T, typename R = void>
using if_container = detail::if_contains<container_types, T, R>;
template <typename T, typename R = void>
using if_composite = detail::if_contains<composite_types, T, R>;

} // namespace apache::thrift::type
