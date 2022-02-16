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

#include <thrift/lib/cpp2/type/detail/Get.h>

namespace apache {
namespace thrift {
namespace type {

// field_tag_t<StructTag, Id> is an alias for the field_t of the field in given
// thrift struct where corresponding field id == Id, or for void if there is no
// such field.
//
// Compile-time complexity: O(logN) (With O(NlogN) preparation per TU that used
// this alias since fatal::sort<fields> will be instantiated once in each TU)
template <class StructTag, FieldId Id>
using field_tag_t = typename detail::field_tag<StructTag, Id>::type;

} // namespace type
} // namespace thrift
} // namespace apache
