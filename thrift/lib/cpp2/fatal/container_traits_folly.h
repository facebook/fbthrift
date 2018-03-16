/*
 * Copyright 2016 Facebook, Inc.
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
#ifndef THRIFT_FATAL_CONTAINER_TRAITS_FOLLY_H_
#define THRIFT_FATAL_CONTAINER_TRAITS_FOLLY_H_ 1

#include <thrift/lib/cpp2/fatal/reflection.h>

#include <folly/FBString.h>
#include <folly/container/F14Map.h>
#include <folly/container/F14Set.h>
#include <folly/small_vector.h>
#include <folly/sorted_vector_types.h>

namespace apache {
namespace thrift {

template <typename C, typename T, typename A, typename S>
struct thrift_string_traits<folly::basic_fbstring<C, T, A, S>>
    : thrift_string_traits_std<folly::basic_fbstring<C, T, A, S>> {};

template <class T, std::size_t M, class A, class B, class C>
struct thrift_list_traits<folly::small_vector<T, M, A, B, C>>
    : thrift_list_traits_std<folly::small_vector<T, M, A, B, C>> {};

template <typename T, typename C, typename A, typename G>
struct thrift_set_traits<folly::sorted_vector_set<T, C, A, G>>
    : thrift_set_traits_std<folly::sorted_vector_set<T, C, A, G>> {};

template <typename K, typename V, typename C, typename A, typename G>
struct thrift_map_traits<folly::sorted_vector_map<K, V, C, A, G>>
    : thrift_map_traits_std<folly::sorted_vector_map<K, V, C, A, G>> {};

template <class K, class H, class E, class A>
struct thrift_set_traits<folly::F14ValueSet<K, H, E, A>>
    : thrift_set_traits_std<folly::F14ValueSet<K, H, E, A>> {};

template <class K, class H, class E, class A>
struct thrift_set_traits<folly::F14NodeSet<K, H, E, A>>
    : thrift_set_traits_std<folly::F14NodeSet<K, H, E, A>> {};

template <class K, class H, class E, class A>
struct thrift_set_traits<folly::F14VectorSet<K, H, E, A>>
    : thrift_set_traits_std<folly::F14VectorSet<K, H, E, A>> {};

template <class K, class T, class H, class E, class A>
struct thrift_map_traits<folly::F14ValueMap<K, T, H, E, A>>
    : thrift_map_traits_std<folly::F14ValueMap<K, T, H, E, A>> {};

template <class K, class T, class H, class E, class A>
struct thrift_map_traits<folly::F14NodeMap<K, T, H, E, A>>
    : thrift_map_traits_std<folly::F14NodeMap<K, T, H, E, A>> {};

template <class K, class T, class H, class E, class A>
struct thrift_map_traits<folly::F14VectorMap<K, T, H, E, A>>
    : thrift_map_traits_std<folly::F14VectorMap<K, T, H, E, A>> {};

} // namespace thrift
} // namespace apache

#endif // THRIFT_FATAL_CONTAINER_TRAITS_FOLLY_H_
