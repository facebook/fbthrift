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
#include <thrift/lib/cpp2/op/DeterministicAccumulator.h>
#include <thrift/lib/cpp2/op/StdHasher.h>
#include <thrift/lib/cpp2/op/detail/HashProtocol.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache::thrift::op::detail {

// By default, pass the value directly to the accumulator.
template <typename Accumulator, typename T>
void accumulateHash(type::all_c, Accumulator& accumulator, const T& value) {
  accumulator.combine(value);
}

template <typename Accumulator, typename T>
void accumulateHash(type::string_c, Accumulator& accumulator, const T& value) {
  accumulator.combine(folly::ByteRange(
      reinterpret_cast<const unsigned char*>(value.data()), value.size()));
}

template <
    typename ValTag,
    template <typename...>
    typename ListT,
    typename Accumulator,
    typename T>
void accumulateHash(
    type::list<ValTag, ListT>, Accumulator& accumulator, const T& value) {
  auto guard = makeOrderedHashGuard(accumulator);
  for (const auto& i : value) {
    accumulateHash(ValTag{}, accumulator, i);
  }
}

template <
    typename KeyTag,
    template <typename...>
    typename SetT,
    typename Accumulator,
    typename T>
void accumulateHash(
    type::set<KeyTag, SetT>, Accumulator& accumulator, const T& value) {
  auto guard = makeUnorderedHashGuard(accumulator);
  for (const auto& i : value) {
    accumulateHash(KeyTag{}, accumulator, i);
  }
}

template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT,
    typename Accumulator,
    typename T>
void accumulateHash(
    type::map<KeyTag, ValTag, MapT>, Accumulator& accumulator, const T& value) {
  auto guard = makeUnorderedHashGuard(accumulator);
  for (const auto& i : value) {
    auto pairGuard = makeOrderedHashGuard(accumulator);
    accumulateHash(KeyTag{}, accumulator, i.first);
    accumulateHash(ValTag{}, accumulator, i.second);
  }
}

template <typename Accumulator, typename T>
void accumulateHash(
    type::structured_c, Accumulator& accumulator, const T& value) {
  detail::HashProtocol protocol(accumulator);
  value.write(&protocol);
}

template <typename Tag>
struct Hash {
  template <typename T, typename Accumulator>
  void operator()(const T& value, Accumulator& accumulator) const {
    accumulateHash(Tag{}, accumulator, value);
  }
  template <typename T = type::native_type<Tag>>
  auto operator()(const T& value) const {
    // TODO(afuller): Only use an accumulator for composite types.
    auto accumulator = makeDeterministicAccumulator<StdHasher>();
    accumulateHash(Tag{}, accumulator, value);
    return std::move(accumulator.result()).getResult();
  }
};

} // namespace apache::thrift::op::detail
