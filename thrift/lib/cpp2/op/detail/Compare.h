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

#include <cmath>

#include <folly/CPortability.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache::thrift::op::detail {

template <typename Tag>
struct EqualTo {
  static_assert(type::is_concrete_v<Tag>);
  template <typename T1 = type::native_type<Tag>, typename T2 = T1>
  constexpr bool operator()(const T1& lhs, const T2& rhs) const {
    if constexpr (type::is_a_v<Tag, type::string_c>) {
      return StringTraits<T1>::isEqual(lhs, rhs);
    } else {
      // Use the native c++ operator by default.
      return lhs == rhs;
    }
  }
};

template <typename Adapter, typename Tag>
struct EqualTo<type::adapted<Adapter, Tag>> {
  // TODO(afuller): Implement.
};

template <typename Tag>
struct IdenticalTo : EqualTo<Tag> {
  // Identical is the same as equal for integral, enum and string types.
  static_assert(
      type::is_a_v<Tag, type::integral_c> || type::is_a_v<Tag, type::enum_c> ||
      type::is_a_v<Tag, type::string_c> ||
      // TODO(afuller): Implement proper specializations for all container
      // types.
      type::container_types::contains<Tag>() ||
      // TODO(afuller): Implement proper specializations for all structured
      // types.
      type::structured_types::contains<Tag>());
};

template <typename Adapter, typename Tag>
struct IdenticalTo<type::adapted<Adapter, Tag>> {
  // TODO(afuller): Implement.
};

template <typename F, typename I>
struct FloatIdenticalTo {
  bool operator()(F lhs, F rhs) const {
    // NOTE: Thrift specifies that all NaN variations are considered
    // 'identical'; however, we do not implement that here for performance
    // reasons.
    return folly::bit_cast<I>(lhs) == folly::bit_cast<I>(rhs);
  }
};
template <>
struct IdenticalTo<type::float_t> : FloatIdenticalTo<float, int32_t> {};
template <>
struct IdenticalTo<type::double_t> : FloatIdenticalTo<double, int64_t> {};

template <typename ValTag, template <typename...> typename ListT>
struct IdenticalTo<type::list<ValTag, ListT>> {
  template <typename T = type::native_type<type::list<ValTag, ListT>>>
  bool operator()(const T& lhs, const T& rhs) const {
    if (&lhs == &rhs) {
      return true;
    }
    if (lhs.size() != rhs.size()) {
      return false;
    }
    return std::equal(
        lhs.begin(), lhs.end(), rhs.begin(), IdenticalTo<ValTag>());
  }
};

} // namespace apache::thrift::op::detail
