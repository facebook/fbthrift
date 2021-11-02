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

// Operations supported by all ThriftType values.
#pragma once

#include <algorithm>

#include <folly/lang/Bits.h>
#include <thrift/lib/cpp2/type/ThriftType.h>

namespace apache::thrift::op {

// Default to the native c++ notion of equal.
template <typename TT>
struct equal_to {
  static_assert(type::is_concrete_type_v<TT>);

  template <typename T = typename TT::standard_type>
  constexpr bool operator()(const T& lhs, const T& rhs) const {
    static_assert(type::is_standard_type<TT, T>::value);
    return lhs == rhs;
  }
};

template <typename TT>
constexpr equal_to<TT> equal;

// identical and equal are the same for most types.
template <typename TT>
struct identical_to : equal_to<TT> {};

template <typename TT>
constexpr identical_to<TT> identical;

// Floating point overrides.
// NOTE: Technically Thrift considers all NaN variations identical.
template <>
struct identical_to<type::float_t> {
  bool operator()(float lhs, float rhs) const {
    return folly::bit_cast<int32_t>(lhs) == folly::bit_cast<int32_t>(rhs);
  }
};
template <>
struct identical_to<type::double_t> {
  bool operator()(double lhs, double rhs) const {
    return folly::bit_cast<int64_t>(lhs) == folly::bit_cast<int64_t>(rhs);
  }
};

// TODO(afuller): Implement overrides for lists, sets, maps and structured
// types, so that identical_to works correctly for floating point types.

} // namespace apache::thrift::op
