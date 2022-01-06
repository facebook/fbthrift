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

#include <cmath>

#include <folly/Range.h>
#include <thrift/lib/cpp2/hash/StdHasher.h>
#include <thrift/lib/cpp2/op/DeterministicAccumulator.h>
#include <thrift/lib/cpp2/op/detail/HashProtocol.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache::thrift::op::detail {

template <typename Tag>
struct HashImpl {
 public:
  template <typename HashAccumulator, typename T = type::native_type<Tag>>
  constexpr void operator()(
      HashAccumulator& accumulator, const T& value) const {
    accumulator.combine(value);
  }
};

template <>
struct HashImpl<type::string_t> {
  template <
      typename Accumulator,
      typename T = type::native_type<type::string_t>>
  constexpr void operator()(Accumulator& accumulator, const T& value) const {
    accumulator.combine(folly::ByteRange(
        reinterpret_cast<const unsigned char*>(value.data()), value.size()));
  }
};

template <>
struct HashImpl<type::binary_t> : HashImpl<type::string_t> {};

template <typename ValTag>
struct HashImpl<type::list<ValTag>> {
  template <
      typename Accumulator,
      typename T = type::native_type<type::list<ValTag>>>
  constexpr void operator()(Accumulator& accumulator, const T& value) const {
    auto guard = makeOrderedHashGuard(accumulator);
    for (const auto& i : value) {
      HashImpl<ValTag>{}(accumulator, i);
    }
  }
};

template <typename ValTag>
struct HashImpl<type::set<ValTag>> {
  template <
      typename Accumulator,
      typename T = type::native_type<type::set<ValTag>>>
  constexpr void operator()(Accumulator& accumulator, const T& value) const {
    auto guard = makeUnorderedHashGuard(accumulator);
    for (const auto& i : value) {
      HashImpl<ValTag>{}(accumulator, i);
    }
  }
};

template <typename KeyTag, typename ValTag>
struct HashImpl<type::map<KeyTag, ValTag>> {
  template <
      typename Accumulator,
      typename T = type::native_type<type::map<KeyTag, ValTag>>>
  constexpr void operator()(Accumulator& accumulator, const T& value) const {
    auto guard = makeUnorderedHashGuard(accumulator);
    for (const auto& i : value) {
      auto pairGuard = makeOrderedHashGuard(accumulator);
      HashImpl<KeyTag>{}(accumulator, i.first);
      HashImpl<ValTag>{}(accumulator, i.second);
    }
  }
};

template <typename StructType>
struct HashImpl<type::struct_t<StructType>> {
  template <
      typename Accumulator,
      typename T = type::native_type<type::struct_t<StructType>>>
  constexpr void operator()(Accumulator& accumulator, const T& value) const {
    detail::HashProtocol protocol(accumulator);
    value.write(&protocol);
  }
};

template <typename Tag>
struct Hash {
  template <typename T, typename Accumulator>
  void operator()(const T& value, Accumulator& accumulator) const {
    HashImpl<Tag>{}(accumulator, value);
  }
  template <typename T = type::native_type<Tag>>
  auto operator()(const T& value) const {
    auto accumulator = makeDeterministicAccumulator<hash::StdHasher>();
    operator()(value, accumulator);
    return std::move(accumulator.result()).getResult();
  }
};

} // namespace apache::thrift::op::detail
