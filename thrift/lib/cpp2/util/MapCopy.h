/*
 * Copyright 2016-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <vector>
#include <map>
#include <cstdint>

#include <folly/lang/Bits.h>

namespace apache { namespace thrift {

namespace detail {
// minimum size of an std::map before an intermediary vector
// and level order insertion is used to build the map
// see map_insertion_order_benchmark.cpp
const std::size_t map_size_level_insertion_breakpoint = 100;
}

// only for std::map (or maps backed by a binary tree) pls
template <typename OrderedMap, typename OrderedVect>
std::size_t move_ordered_map_vec(OrderedMap& dest, OrderedVect&& source) {
  const std::size_t size = source.size();
  // height = height of balanced tree serialized into (ordered) source
  // height = log2(size)
  auto const height = static_cast<std::size_t>(folly::findLastSet(size));

  for(std::size_t level = 0; level <= height; ++level) {
    // offset = index to the first element in this level of the tree
    // offset = pow(2, (height - level) - 1)
    const std::size_t offset =
      (static_cast<std::size_t>(1) << (height - level)) - 1;
    // gap = number of elements to skip to get to next node in this tree level
    // gap = pow(2, (height - level + 1))
    const std::size_t gap =
      (static_cast<std::size_t>(1) << (height - level + 1));

    for(std::size_t idx = offset; idx < size; idx += gap) {
      assert(idx < size);
      dest.emplace(std::move(source[idx]));
      assert(dest.size() <= size);
    }
  }

  assert(dest.size() == size);
  return dest.size();
}

// version which does not check its type
template <
  typename Map,
  typename KeyDeserializer,
  typename ValDeserializer
>
std::size_t deserialize_known_length_map_impl(
  Map& map,
  const std::uint32_t map_size,
  KeyDeserializer const& kr,
  ValDeserializer const& vr
) {
  // some map implementations do not provide a Map::key_type or Map::mapped_type
  // so must exactract through this
  using K = typename std::decay<decltype(map.begin()->first)>::type;

  for(auto i = map_size; i--;) {
    K key;
    kr(key);
    vr(map[std::move(key)]);
  }
  return map_size;
}

// Specialization for std::map (assumes std::map is backed by an RB tree)
// Will use level-order insertion for large maps to avoid
// red-black tree rotation cost. If the map is not large enough, then
// the standard method of read key, read value, insert into map will be used
// to construct the map.
// For larger maps, the cost of red/black tree rotations can be expensive,
// so constructing an intermediary vector, and inserting k/v pairs in level
// order is cheaper than a straight insertion, and has the added benefit that
// the backing RB tree is perfectly balanced.
// Assumes that the keys and values held in the map are cheap to move
template <
  typename K,
  typename V,
  typename KeyDeserializer,
  typename ValDeserializer
>
std::size_t
deserialize_known_length_map(
  std::map<K, V>& map,
  const std::uint32_t map_size,
  KeyDeserializer const& kr,
  ValDeserializer const& vr
) {
  if(map_size < detail::map_size_level_insertion_breakpoint) {
    return deserialize_known_length_map_impl(map, map_size, kr, vr);
  }

  std::vector<std::pair<K, V>> flattened_bst;
  flattened_bst.reserve(map_size);

  for(auto i = map_size; i--;) {
    flattened_bst.emplace_back();
    auto& back = flattened_bst.back();
    kr(back.first);
    vr(back.second);
  }
  return move_ordered_map_vec(map, flattened_bst);
}

// Generic implementation: standard in-order Thrift map deserialization
template <
  typename Map,
  typename KeyDeserializer,
  typename ValDeserializer
>
std::size_t deserialize_known_length_map(
  Map& map,
  const std::uint32_t map_size,
  KeyDeserializer const& kr,
  ValDeserializer const& vr
) {
  return deserialize_known_length_map_impl(
    map, map_size, kr, vr
  );
}

} } // namespace apache::thrift
