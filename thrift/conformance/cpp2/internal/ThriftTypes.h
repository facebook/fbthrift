/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <map>
#include <set>
#include <vector>

#include <fatal/type/find.h>
#include <fatal/type/sort.h>
#include <fatal/type/transform.h>
#include <folly/Traits.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>

namespace apache::thrift::conformance::detail {

template <BaseType B>
struct has_base_type {
  template <typename T>
  using apply = std::bool_constant<T::kBaseType == B>;
};

struct vector_of {
  template <typename T>
  using apply = std::vector<T>;
};

struct set_of {
  template <typename T>
  using apply = std::set<T>;
};

template <typename K>
struct map_to {
  template <typename V>
  using apply = std::map<K, V>;
};

template <typename T, typename = void>
struct is_concrete_type : std::false_type {};
template <typename T>
struct is_concrete_type<T, folly::void_t<typename T::native_type>>
    : std::true_type {};
template <typename T>
inline constexpr bool is_concrete_type_v = detail::is_concrete_type<T>::value;
template <typename... Ts>
using if_all_concrete = std::enable_if_t<(is_concrete_type_v<Ts> && ...)>;

template <typename... Ts>
struct types {
  template <typename T>
  static constexpr bool contains = fatal::contains<types, T>::value;

  template <BaseType B>
  static constexpr bool contains_bt =
      !fatal::empty<fatal::filter<types, has_base_type<B>>>::value;

  // The Ith type.
  template <size_t I>
  using at = typename fatal::at<types, I>;

  // Converts the type list to a type list of the given types.
  template <template <typename...> typename T>
  using as = T<Ts...>;
};

template <typename Ts, typename T, typename R = void>
using if_contains_bt =
    std::enable_if_t<Ts::template contains_bt<T::kBaseType>, R>;

template <BaseType B>
struct base_type {
  static constexpr BaseType kBaseType = B;
};

template <typename Base, typename NativeTs>
struct concrete_type : Base {
  using native_types = NativeTs;
  using native_type = fatal::first<NativeTs>;
};

template <typename T, typename A>
using expand_types = fatal::transform<typename T::native_types, A>;

template <BaseType B, typename... NativeTs>
using primitive_type = concrete_type<base_type<B>, types<NativeTs...>>;

template <typename VT>
struct base_list : base_type<BaseType::List> {
  using value_type = VT;
};

template <typename VT, typename = void>
struct list : base_list<VT> {};

template <typename VT>
struct list<VT, if_all_concrete<VT>>
    : concrete_type<base_list<VT>, expand_types<VT, vector_of>> {};

template <typename VT>
struct base_set : base_type<BaseType::Set> {
  using value_type = VT;
};

template <typename VT, typename = void>
struct set : base_set<VT> {};

template <typename VT>
struct set<VT, if_all_concrete<VT>>
    : concrete_type<base_set<VT>, expand_types<VT, set_of>> {};

template <typename KT, typename VT>
struct base_map : base_type<BaseType::Map> {
  using key_type = KT;
  using mapped_type = VT;
};

template <typename KT, typename VT, typename = void>
struct map : base_map<KT, VT> {};

template <typename KT, typename VT>
struct map<KT, VT, if_all_concrete<KT, VT>>
    : concrete_type<
          base_map<KT, VT>,
          expand_types<VT, map_to<typename KT::native_type>>> {};

} // namespace apache::thrift::conformance::detail
