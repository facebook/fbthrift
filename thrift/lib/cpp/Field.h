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

#include <cstdint>
#include <type_traits>

namespace apache {
namespace thrift {
namespace type {
// Forward declare to avoid circular dep with thrift/lib/thrift/id.thrift.
enum class FieldId : ::std::int16_t;
} // namespace type

// Runtime and compile time representations for a field id.
using FieldId = type::FieldId;
template <FieldId id>
using field_id_tag = std::integral_constant<FieldId, id>;
template <std::underlying_type_t<FieldId> id>
using field_id = field_id_tag<FieldId(id)>;

// Runtime and compile time representations for a field ordinal.
enum class FieldOrdinal : uint16_t {};
template <FieldOrdinal ord>
using field_ordinal_tag = std::integral_constant<FieldOrdinal, ord>;
template <std::underlying_type_t<FieldOrdinal> ord>
using field_ordinal = field_ordinal_tag<FieldOrdinal(ord)>;

// TODO(dokwon): Change FieldContext to use strong FieldId type.
template <typename Struct, int16_t FieldId>
struct FieldContext {
  static constexpr int16_t kFieldId = FieldId;
  Struct& object;
};

} // namespace thrift
} // namespace apache
