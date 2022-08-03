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

// TODO(afuller): Update all references and remove these mappings.
#pragma once

#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/type/Id.h>

namespace apache {
namespace thrift {
namespace type {

template <typename Tag>
FOLLY_INLINE_VARIABLE constexpr auto field_size_v = op::size_v<Tag>;
template <class Tag, class Id>
using get_field_id = op::get_field_id<Tag, Id>;
template <class Tag, class Id>
using get_field_tag = op::get_field_tag<Tag, Id>;
template <class Tag, class Id>
using get_field_native_type = native_type<get_field_tag<Tag, Id>>;
template <class Tag, class T>
using get_field_ordinal = op::get_ordinal<Tag, T>;
template <class Tag, class T>
using get_field_type_tag = op::get_type_tag<Tag, T>;
template <class Tag, class T>
using get_field_ident = op::get_ident<Tag, T>;

} // namespace type
} // namespace thrift
} // namespace apache
