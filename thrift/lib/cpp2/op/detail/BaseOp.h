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

#include <thrift/lib/cpp2/op/Clear.h>
#include <thrift/lib/cpp2/op/Compare.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/detail/RuntimeType.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

// Ops all Thrift types support.
template <typename Tag>
struct BaseAnyOp : type::detail::BaseErasedOp {
  // Blind convert the pointer to the native type.
  using T = type::native_type<Tag>;
  static T& ref(void* ptr) { return *static_cast<T*>(ptr); }
  static const T& ref(const void* ptr) { return *static_cast<const T*>(ptr); }

  static bool empty(const void* ptr) { return op::isEmpty<Tag>(ref(ptr)); }
  static void clear(void* ptr) { op::clear<Tag>(ref(ptr)); }
  static bool identical(const void* lhs, const type::detail::Ptr& rhs) {
    // Caller should have already checked the types match.
    assert(rhs.type()->thriftType == Tag{});
    return op::identical<Tag>(ref(lhs), rhs.as<Tag>());
  }
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
