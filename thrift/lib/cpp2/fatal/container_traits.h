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
#ifndef THRIFT_FATAL_CONTAINER_TRAITS_H_
#define THRIFT_FATAL_CONTAINER_TRAITS_H_ 1

#include <deque>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <thrift/lib/cpp2/fatal/reflection.h>

namespace apache {
namespace thrift {

template <typename C, typename T, typename A>
struct thrift_string_traits<std::basic_string<C, T, A>>
    : thrift_string_traits_std<std::basic_string<C, T, A>> {};

template <typename T, typename A>
struct thrift_list_traits<std::deque<T, A>>
    : thrift_list_traits_std<std::deque<T, A>> {};

template <typename T, typename A>
struct thrift_list_traits<std::vector<T, A>>
    : thrift_list_traits_std<std::vector<T, A>> {};

template <typename K, typename C, typename A>
struct thrift_set_traits<std::set<K, C, A>>
    : thrift_set_traits_std<std::set<K, C, A>> {};

template <typename K, typename H, typename E, typename A>
struct thrift_set_traits<std::unordered_set<K, H, E, A>>
    : thrift_set_traits_std<std::unordered_set<K, H, E, A>> {};

template <typename K, typename T, typename C, typename A>
struct thrift_map_traits<std::map<K, T, C, A>>
    : thrift_map_traits_std<std::map<K, T, C, A>> {};

template <typename K, typename T, typename H, typename E, typename A>
struct thrift_map_traits<std::unordered_map<K, T, H, E, A>>
    : thrift_map_traits_std<std::unordered_map<K, T, H, E, A>> {};

} // namespace thrift
} // namespace apache

#endif // THRIFT_FATAL_CONTAINER_TRAITS_H_
