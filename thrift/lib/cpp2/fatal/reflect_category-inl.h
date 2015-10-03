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
#ifndef THRIFT_FATAL_REFLECT_CATEGORY_INL_H_
#define THRIFT_FATAL_REFLECT_CATEGORY_INL_H_ 1

#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace apache { namespace thrift {
namespace detail {

template <typename T>
struct reflect_category_impl {
  using type = typename std::conditional<
    is_reflectable_struct<T>::value,
    std::integral_constant<thrift_category, thrift_category::structs>,
    typename std::conditional<
      is_reflectable_enum<T>::value,
      std::integral_constant<thrift_category, thrift_category::enums>,
      typename std::conditional<
        is_reflectable_union<T>::value,
        std::integral_constant<thrift_category, thrift_category::unions>,
        typename get_thrift_category<T>::type
      >::type
    >::type
  >::type;
};

} // namespace detail {

template <typename T, typename A>
struct get_thrift_category<std::vector<T, A>> {
  using type = std::integral_constant<thrift_category, thrift_category::lists>;
};

template <typename K, typename C, typename A>
struct get_thrift_category<std::set<K, C, A>> {
  using type = std::integral_constant<thrift_category, thrift_category::sets>;
};

template <typename K, typename H, typename E, typename A>
struct get_thrift_category<std::unordered_set<K, H, E, A>> {
  using type = std::integral_constant<thrift_category, thrift_category::sets>;
};

template <typename K, typename T, typename C, typename A>
struct get_thrift_category<std::map<K, T, C, A>> {
  using type = std::integral_constant<thrift_category, thrift_category::maps>;
};

template <typename K, typename T, typename H, typename E, typename A>
struct get_thrift_category<std::unordered_map<K, T, H, E, A>> {
  using type = std::integral_constant<thrift_category, thrift_category::maps>;
};

}} // apache::thrift

#endif // THRIFT_FATAL_REFLECT_CATEGORY_INL_H_
