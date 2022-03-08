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

#include <thrift/lib/cpp/FieldId.h>
#include <thrift/lib/cpp2/type/detail/Field.h>

namespace apache {
namespace thrift {
namespace type {

// The type tag for the given type::field_t.
template <typename Tag>
using field_type_tag = typename detail::field_to_tag::apply<Tag>::type;

// The FieldId for the given type::field_t
template <typename Tag>
constexpr FieldId field_id_v = FieldId(detail::field_to_id::apply<Tag>::value);

// field_tag_t<StructTag, Id> is an alias for the field_t of the field in given
// thrift struct where corresponding field id == Id, or for void if there is no
// such field.
//
// Compile-time complexity: O(logN) (With O(NlogN) preparation per TU that used
// this alias since fatal::sort<fields> will be instantiated once in each TU)
template <class StructTag, FieldId Id>
using field_tag_by_id = typename detail::field_tag<StructTag, Id>::type;

} // namespace type
} // namespace thrift
} // namespace apache
