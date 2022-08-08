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

// Types use for identifying Thrift definitions.
//
// Thrift supports a number of different types of identifiers, including
//  - FieldId - The numeric identifier for a field.
//  - Ident - The assigned 'name' for a definition.
//  - Oridnal - The 1-based order at which a definition was defined in the
//  IDL/AST.
#pragma once

#include <cstdint>
#include <type_traits>

#include <folly/Traits.h>
#include <folly/Utility.h>
#include <thrift/lib/cpp2/type/Tag.h>

namespace apache {
namespace thrift {
namespace type {

// Runtime and compile time representations for an ordinal.
enum class Ordinal : uint16_t {};
template <Ordinal ord>
using ordinal_tag = std::integral_constant<Ordinal, ord>;
template <std::underlying_type_t<Ordinal> ord>
using ordinal = ordinal_tag<Ordinal(ord)>;
template <typename Id>
FOLLY_INLINE_VARIABLE constexpr bool is_ordinal_v = false;
template <Ordinal ord>
FOLLY_INLINE_VARIABLE constexpr bool
    is_ordinal_v<std::integral_constant<Ordinal, ord>> = true;

// Runtime and compile time representations for a field id.
enum class FieldId : int16_t; // defined in id.thrift
template <FieldId id>
using field_id_tag = std::integral_constant<FieldId, id>;
template <std::underlying_type_t<FieldId> id>
using field_id = field_id_tag<FieldId(id)>;
template <typename Id>
FOLLY_INLINE_VARIABLE constexpr bool is_field_id_v = false;
template <FieldId id>
FOLLY_INLINE_VARIABLE constexpr bool
    is_field_id_v<std::integral_constant<FieldId, id>> = true;

// Helpers for ident tags defined in code gen.
template <typename Id, typename = void>
FOLLY_INLINE_VARIABLE constexpr bool is_ident_v = false;
template <typename Id>
FOLLY_INLINE_VARIABLE constexpr bool is_ident_v<
    Id,
    decltype(__fbthrift_check_whether_type_is_ident_via_adl(
        FOLLY_DECLVAL(Id &&)))> = true;

namespace detail {
// We can't use std::is_base_of since it's undefined behavior to use it on
// incomplete type, and ident is incomplete type.
void is_type_tag_impl(all_c);
void is_type_tag_impl(void_t);
void is_type_tag_impl(service_c);
template <class Id, class = void>
FOLLY_INLINE_VARIABLE constexpr bool is_type_tag_v = false;
template <class Id>
FOLLY_INLINE_VARIABLE constexpr bool
    is_type_tag_v<Id, decltype(is_type_tag_impl(FOLLY_DECLVAL(Id &&)))> = true;
} // namespace detail

// If the given type can be used to identify a definition.
template <typename Id>
FOLLY_INLINE_VARIABLE constexpr bool is_id_v = detail::is_type_tag_v<Id> ||
    is_field_id_v<Id> || is_ordinal_v<Id> || is_ident_v<Id>;

} // namespace type
} // namespace thrift
} // namespace apache
