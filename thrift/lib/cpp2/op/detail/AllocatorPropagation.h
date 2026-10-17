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

#include <memory>
#include <type_traits>

namespace apache::thrift::detail {

template <typename Cont, typename Elem, typename Enable = void>
inline constexpr bool alloc_is_recursive = false;
template <typename Cont, typename Elem>
inline constexpr bool alloc_is_recursive<
    Cont,
    Elem,
    std::void_t<typename Cont::allocator_type, typename Elem::allocator_type>> =
    std::uses_allocator_v<Elem, typename Cont::allocator_type>;
template <typename Cont, typename Elem>
inline constexpr bool alloc_should_propagate = alloc_is_recursive<Cont, Elem> &&
    !std::uses_allocator_v<Cont, std::allocator<char>>;

template <typename Map>
inline constexpr bool alloc_should_propagate_map =
    alloc_should_propagate<Map, typename Map::key_type> ||
    alloc_should_propagate<Map, typename Map::mapped_type>;

template <typename C>
typename C::value_type default_set_element(C& c) {
  if constexpr (alloc_should_propagate<C, typename C::value_type>) {
    return typename C::value_type(c.get_allocator());
  } else {
    return typename C::value_type{};
  }
}
template <typename Map>
typename Map::key_type default_map_key(Map& m) {
  if constexpr (alloc_should_propagate<Map, typename Map::key_type>) {
    return typename Map::key_type(m.get_allocator());
  } else {
    return typename Map::key_type{};
  }
}
template <typename C>
typename C::mapped_type default_map_value(C& map) {
  if constexpr (alloc_should_propagate<C, typename C::mapped_type>) {
    return typename C::mapped_type(map.get_allocator());
  } else {
    return typename C::mapped_type{};
  }
}

} // namespace apache::thrift::detail
