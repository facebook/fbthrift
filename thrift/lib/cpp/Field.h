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

// TODO(dokwon): Change FieldContext to use strong FieldId type.
template <typename Struct, int16_t FieldId>
struct FieldContext {
  static constexpr int16_t kFieldId = FieldId;
  Struct& object;
};

namespace type {
using field_id_u_t = int16_t;
template <field_id_u_t id>
using field_id_u_c = std::integral_constant<field_id_u_t, id>;
} // namespace type

enum class FieldId : type::field_id_u_t {};

template <FieldId Id>
using FieldIdTag = type::field_id_u_c<static_cast<type::field_id_u_t>(Id)>;

enum class FieldOrdinal : uint16_t {};

} // namespace thrift
} // namespace apache
