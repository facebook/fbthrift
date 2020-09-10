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

#include <fatal/type/array.h>
#include <fatal/type/find.h>
#include <fatal/type/sequence.h>
#include <fatal/type/sort.h>
#include <fatal/type/transform.h>
#include <fmt/core.h>
#include <folly/CPortability.h>
#include <folly/String.h>
#include <folly/Traits.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

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
  template <BaseType B>
  static constexpr bool contains_bt =
      !fatal::empty<fatal::filter<types, has_base_type<B>>>::value;

  // TODO(afuller): Make work for list of arbitrary thrift types, instead
  // of just the base thrift types (which are the only ones currently defined).
  template <typename T>
  static constexpr bool contains = contains_bt<T::kBaseType>;

  // The Ith type.
  template <size_t I>
  using at = typename fatal::at<types, I>;

  // Converts the type list to a type list of the given types.
  template <template <typename...> typename T>
  using as = T<Ts...>;
};

template <typename Ts, typename T, typename R = void>
using if_contains = std::enable_if_t<Ts::template contains<T>, R>;

template <BaseType B>
struct base_type {
  static constexpr BaseType kBaseType = B;
  FOLLY_EXPORT static const std::string& getName() {
    static std::string kValue = []() {
      std::string name;
      if (const char* cname = TEnumTraits<BaseType>::findName(B)) {
        name = cname;
        folly::toLowerAscii(name);
      }
      return name;
    }();
    return kValue;
  }
};

template <typename Base, typename NativeTs>
struct concrete_type : Base {
  using native_types = NativeTs;
  using native_type = fatal::first<NativeTs>;
};

template <BaseType B, typename T>
struct cpp_type : concrete_type<base_type<B>, types<T>> {
  FOLLY_EXPORT static const std::string& getName() {
    static const std::string kName = []() {
      // TODO(afuller): Add an Any type name annotation, and
      // use that.
      if constexpr (B == BaseType::Enum) {
        using info = reflect_enum<T>;
        using module = reflect_module<typename info::module>;
        return fmt::format(
            "{}.{}",
            fatal::z_data<typename module::name>(),
            fatal::z_data<typename info::traits::name>());
      } else if constexpr (B == BaseType::Union) {
        using info = reflect_variant<T>;
        using module = reflect_module<typename info::module>;
        return fmt::format(
            "{}.{}",
            fatal::z_data<typename module::name>(),
            fatal::z_data<typename info::traits::name>());

      } else {
        using info = reflect_struct<T>;
        using module = reflect_module<typename info::module>;
        return fmt::format(
            "{}.{}",
            fatal::z_data<typename module::name>(),
            fatal::z_data<typename info::name>());
      }
    }();
    return kName;
  }
}; // namespace apache::thrift::conformance::detail

template <typename T, typename A>
using expand_types = fatal::transform<typename T::native_types, A>;

template <BaseType B, typename... NativeTs>
using primitive_type = concrete_type<base_type<B>, types<NativeTs...>>;

template <typename VT>
struct base_list : base_type<BaseType::List> {
  using value_type = VT;

  FOLLY_EXPORT static const std::string& getName() {
    static const std::string kName = fmt::format("list<{}>", VT::getName());
    return kName;
  }
};

template <typename VT, typename = void>
struct list : base_list<VT> {};

template <typename VT>
struct list<VT, if_all_concrete<VT>>
    : concrete_type<base_list<VT>, expand_types<VT, vector_of>> {};

template <typename VT>
struct base_set : base_type<BaseType::Set> {
  using value_type = VT;
  FOLLY_EXPORT static const std::string& getName() {
    static const std::string kName = fmt::format("set<{}>", VT::getName());
    return kName;
  }
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
  FOLLY_EXPORT static const std::string& getName() {
    static const std::string kName =
        fmt::format("map<{}, {}>", KT::getName(), VT::getName());
    return kName;
  }
};

template <typename KT, typename VT, typename = void>
struct map : base_map<KT, VT> {};

template <typename KT, typename VT>
struct map<KT, VT, if_all_concrete<KT, VT>>
    : concrete_type<
          base_map<KT, VT>,
          expand_types<VT, map_to<typename KT::native_type>>> {};

} // namespace apache::thrift::conformance::detail
