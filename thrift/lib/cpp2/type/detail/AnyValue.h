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

#include <folly/Poly.h>
#include <thrift/lib/cpp2/type/ThriftOp.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache::thrift::type::detail {

template <typename Tag>
struct AnyValueData {
  static_assert(is_concrete_v<Tag>);
  native_type<Tag> data;

  constexpr bool empty() const noexcept { return op::isEmpty<Tag>(data); }
  constexpr void clear() noexcept { op::clear<Tag>(data); }
  constexpr bool identical(const AnyValueData& other) const noexcept {
    return op::identical<Tag>(data, other.data);
  }
};

struct VoidValueData {
  constexpr bool empty() const noexcept { return true; }
  constexpr void clear() noexcept {}
  constexpr bool identical(VoidValueData) const { return true; }
};

struct IAnyValueData {
  // Define the interface of AnyValueData:
  template <class Base>
  struct Interface : Base {
    bool empty() const { return folly::poly_call<0>(*this); }
    void clear() { folly::poly_call<1>(*this); }
    bool identical(const folly::PolySelf<Base>& other) const {
      return folly::poly_call<2>(*this, other);
    }
  };

  template <class T>
  using Members = folly::PolyMembers<&T::empty, &T::clear, &T::identical>;
};

using AnyValueHolder = folly::Poly<IAnyValueData>;

} // namespace apache::thrift::type::detail
