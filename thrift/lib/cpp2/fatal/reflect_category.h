/*
 * Copyright 2015 Facebook, Inc.
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

template <typename C, typename T, typename A>
struct get_thrift_category<std::basic_string<C, T, A>> {
  using type = detail::as_thrift_category<thrift_category::string>;
};

template <>
struct get_thrift_category<folly::fbstring> {
  using type = detail::as_thrift_category<thrift_category::string>;
};

template <typename T, typename A>
struct get_thrift_category<std::vector<T, A>> {
  using type = detail::as_thrift_category<thrift_category::list>;
};

template <typename K, typename C, typename A>
struct get_thrift_category<std::set<K, C, A>> {
  using type = detail::as_thrift_category<thrift_category::set>;
};

template <typename K, typename H, typename E, typename A>
struct get_thrift_category<std::unordered_set<K, H, E, A>> {
  using type = detail::as_thrift_category<thrift_category::set>;
};

template <typename K, typename T, typename C, typename A>
struct get_thrift_category<std::map<K, T, C, A>> {
  using type = detail::as_thrift_category<thrift_category::map>;
};

template <typename K, typename T, typename H, typename E, typename A>
struct get_thrift_category<std::unordered_map<K, T, H, E, A>> {
  using type = detail::as_thrift_category<thrift_category::map>;
};

}} // apache::thrift

#endif // THRIFT_FATAL_REFLECT_CATEGORY_H_
