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

#include <folly/sorted_vector_types.h>

namespace apache { namespace thrift {

// Update `thrift_set_traits` documentation in `reflection.h` after making
// any changes to the interface, and `test_thrift_set_traits` in
// `traits_test_helpers.h`.
template <typename T, typename C, typename A, typename G>
struct thrift_set_traits<folly::sorted_vector_set<T, C, A, G>> {
  using type = folly::sorted_vector_set<T, C, A, G>;

  using key_type = typename type::key_type;
  using value_type = typename type::value_type;
  using size_type = typename type::size_type;
  using iterator = typename type::iterator;
  using const_iterator = typename type::const_iterator;

  static iterator begin(type &what) { return what.begin(); }
  static iterator end(type &what) { return what.end(); }

  static const_iterator cbegin(type const &what) { return what.begin(); }
  static const_iterator begin(type const &what) { return what.begin(); }
  static const_iterator cend(type const &what) { return what.end(); }
  static const_iterator end(type const &what) { return what.end(); }

  static void clear(type &what) { what.clear(); }
  static bool empty(type const &what) { return what.empty(); }
  static iterator find(type &what, key_type const &k) { return what.find(k); }
  static const_iterator find(type const &what, key_type const &k) {
    return what.find(k);
  }
  static iterator insert(
      type &what, const_iterator position, value_type const &val) {
    return what.insert(position, val);
  }
  static iterator insert(
      type &what, const_iterator position, value_type &&val) {
    return what.insert(position, std::move(val));
  }
  static size_type size(type const &what) { return what.size(); }
};

// Update `thrift_map_traits` documentation in `reflection.h` after making
// any changes to the interface, and `test_thrift_map_traits` in
// `traits_test_helpers.h`.
template <typename K, typename V, typename C, typename A, typename G>
struct thrift_map_traits<folly::sorted_vector_map<K, V, C, A, G>> {
  using type = folly::sorted_vector_map<K, V, C, A, G>;

  using key_type = typename type::key_type;
  using mapped_type = typename type::mapped_type;
  using value_type = typename type::value_type;
  using size_type = typename type::size_type;
  using iterator = typename type::iterator;
  using const_iterator = typename type::const_iterator;

  using key_const_reference = key_type const &;
  using mapped_const_reference = mapped_type const &;
  using mapped_reference = mapped_type &;

  static iterator begin(type &what) { return what.begin(); }
  static iterator end(type &what) { return what.end(); }

  static const_iterator cbegin(type const &what) { return what.begin(); }
  static const_iterator begin(type const &what) { return what.begin(); }
  static const_iterator cend(type const &what) { return what.end(); }
  static const_iterator end(type const &what) { return what.end(); }

  static key_const_reference key(const_iterator i) { return i->first; }
  static key_const_reference key(iterator i) { return i->first; }
  static mapped_const_reference mapped(const_iterator i) { return i->second; }
  static mapped_reference mapped(iterator i) { return i->second; }

  static void clear(type &what) { what.clear(); }
  static bool empty(type const &what) { return what.empty(); }
  static iterator find(type &what, key_type const &k) { return what.find(k); }
  static const_iterator find(type const &what, key_type const &k) {
    return what.find(k);
  }
  static mapped_type& get_or_create(type &what, key_type const &k) {
    return what[k];
  }
  static mapped_type& get_or_create(type &what, key_type &&k) {
    return what[std::move(k)];
  }
  static size_type size(type const &what) { return what.size(); }
};

}} // apache::thrift

#endif // THRIFT_FATAL_CONTAINER_TRAITS_FOLLY_H_
