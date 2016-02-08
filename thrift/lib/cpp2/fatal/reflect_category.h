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
#ifndef THRIFT_FATAL_REFLECT_CATEGORY_H_
#define THRIFT_FATAL_REFLECT_CATEGORY_H_ 1

#include <thrift/lib/cpp2/fatal/reflection.h>

#include <folly/FBString.h>

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace apache { namespace thrift {

// Update `thrift_string_traits` documentation in `reflection.h` after making
// any changes to the interface
template <typename C, typename T, typename A>
struct thrift_string_traits<std::basic_string<C, T, A>> {
  using type = std::basic_string<C, T, A>;

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
  static size_type size(type const &what) { return what.size(); }

  static value_type const *data(type const &what) { return what.data(); }
  static value_type const *c_str(type const &what) { return what.c_str(); }
};

template <>
struct thrift_string_traits<folly::fbstring> {
  using type = folly::fbstring;

  using value_type = type::value_type;
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
  static size_type size(type const &what) { return what.size(); }

  static value_type const *data(type const &what) { return what.data(); }
  static value_type const *c_str(type const &what) { return what.c_str(); }
};

// Update `thrift_list_traits` documentation in `reflection.h` after making
// any changes to the interface
template <typename T, typename A>
struct thrift_list_traits<std::vector<T, A>> {
  using type = std::vector<T, A>;

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
  static size_type size(type const &what) { return what.size(); }
};

template <typename K, typename C, typename A>
struct thrift_set_traits<std::set<K, C, A>> {
  using type = std::set<K, C, A>;

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
  static size_type size(type const &what) { return what.size(); }
};

// Update `thrift_set_traits` documentation in `reflection.h` after making
// any changes to the interface
template <typename K, typename H, typename E, typename A>
struct thrift_set_traits<std::unordered_set<K, H, E, A>> {
  using type = std::unordered_set<K, H, E, A>;

  using key_type = typename type::key_type;
  using value_type = typename type::value_type;
  using size_type = typename type::size_type;
  using iterator = typename type::iterator;
  using const_iterator = typename type::const_iterator;

  using value_const_reference = value_type const &;
  using value_reference = value_type &;

  static iterator begin(type &what) { return what.begin(); }
  static iterator end(type &what) { return what.end(); }

  static const_iterator cbegin(type const &what) { return what.begin(); }
  static const_iterator begin(type const &what) { return what.begin(); }
  static const_iterator cend(type const &what) { return what.end(); }
  static const_iterator end(type const &what) { return what.end(); }

  static void clear(type &what) { what.clear(); }
  static bool empty(type const &what) { return what.empty(); }
  static size_type size(type const &what) { return what.size(); }
};

template <typename K, typename T, typename C, typename A>
struct thrift_map_traits<std::map<K, T, C, A>> {
  using type = std::map<K, T, C, A>;

  using key_type = typename type::key_type;
  using mapped_type = typename type::mapped_type;
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
  static size_type size(type const &what) { return what.size(); }
};

// Update `thrift_map_traits` documentation in `reflection.h` after making
// any changes to the interface
template <typename K, typename T, typename H, typename E, typename A>
struct thrift_map_traits<std::unordered_map<K, T, H, E, A>> {
  using type = std::unordered_map<K, T, H, E, A>;

  using key_type = typename type::key_type;
  using mapped_type = typename type::mapped_type;
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
  static size_type size(type const &what) { return what.size(); }
};

}} // apache::thrift

#endif // THRIFT_FATAL_REFLECT_CATEGORY_H_
