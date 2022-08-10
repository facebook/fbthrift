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
#include <thrift/lib/cpp2/type/detail/RuntimeType.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {
using Ptr = type::detail::Ptr;

template <typename Tag, typename T>
struct BaseAnyOp : type::detail::BaseErasedOp {
  // Blind convert the pointer to T
  static T& ref(void* ptr) { return *static_cast<T*>(ptr); }
  static const T& ref(const void* ptr) { return *static_cast<const T*>(ptr); }
  template <typename PTag, typename U = type::native_type<PTag>>
  static const U& ref(const Ptr& ptr) {
    if (ptr.type->cppType == typeid(U)) {
      return *static_cast<const U*>(ptr.ptr);
    }
    bad_type();
  }

  // Try to convert other to T, bad_type() on failure.
  static const T& same(const Ptr& other) { return ref<Tag, T>(other); }

  // Ops all Thrift types support.
  static bool empty(const void* ptr) { return op::isEmpty<Tag>(ref(ptr)); }
  static void clear(void* ptr) { op::clear<Tag>(ref(ptr)); }
  static bool identical(const void* lhs, const Ptr& rhs) {
    // Caller should have already checked the types match.
    assert(rhs.type->thriftType == Tag{});
    return op::identical<Tag>(ref(lhs), same(rhs));
  }
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
