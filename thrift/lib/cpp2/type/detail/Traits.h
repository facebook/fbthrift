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

#include <fmt/core.h>
#include <fatal/type/array.h>
#include <fatal/type/find.h>
#include <fatal/type/sequence.h>
#include <fatal/type/sort.h>
#include <fatal/type/transform.h>
#include <folly/CPortability.h>
#include <folly/String.h>
#include <folly/Traits.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/thrift/gen-cpp2/type_fatal.h>
#include <thrift/lib/thrift/gen-cpp2/type_types.h>

namespace apache::thrift::type::detail {

template <typename TT>
struct traits;

template <BaseType B>
struct has_base_type {
  template <typename TT>
  using apply = std::bool_constant<traits<TT>::kBaseType == B>;
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

template <typename TT, typename = void>
struct is_concrete_type : std::false_type {};
template <typename TT>
struct is_concrete_type<TT, folly::void_t<typename traits<TT>::standard_type>>
    : std::true_type {};
template <typename TT>
inline constexpr bool is_concrete_type_v = is_concrete_type<TT>::value;
template <typename... TTs>
using if_all_concrete = std::enable_if_t<(is_concrete_type_v<TTs> && ...)>;

template <typename... TTs>
struct types {
  template <BaseType B>
  static constexpr bool contains_bt =
      !fatal::empty<fatal::filter<types, has_base_type<B>>>::value;

  // TODO(afuller): Make work for list of arbitrary thrift types, instead
  // of just the base thrift types.
  template <typename TT>
  static constexpr bool contains = contains_bt<traits<TT>::kBaseType>;

  // The Ith type.
  template <size_t I>
  using at = typename fatal::at<types, I>;

  // Converts the type list to a type list of the given types.
  template <template <typename...> typename T>
  using as = T<TTs...>;
};

template <typename Ts, typename TT, typename R = void>
using if_contains = std::enable_if_t<Ts::template contains<TT>, R>;

template <BaseType B>
struct base_type {
  static constexpr BaseType kBaseType = B;
};

template <typename Base, typename StandardTs>
struct concrete_type : Base {
  using standard_types = StandardTs;
  using standard_type = fatal::first<StandardTs>;
};

template <BaseType B, typename T>
struct cpp_type : concrete_type<base_type<B>, types<T>> {};

template <typename TT, typename A>
using expand_types = fatal::transform<typename traits<TT>::standard_types, A>;

template <BaseType B, typename... StandardTs>
using primitive_type = concrete_type<base_type<B>, types<StandardTs...>>;

template <typename VTT>
struct base_list_type : base_type<BaseType::List> {
  using value_type = VTT;
};

template <typename VTT, typename = void>
struct list_type : base_list_type<VTT> {};

template <typename VTT>
struct list_type<VTT, if_all_concrete<VTT>>
    : concrete_type<base_list_type<VTT>, expand_types<VTT, vector_of>> {};

template <typename VTT>
struct base_set_type : base_type<BaseType::Set> {
  using value_type = VTT;
};

template <typename VTT, typename = void>
struct set_type : base_set_type<VTT> {};

template <typename VTT>
struct set_type<VTT, if_all_concrete<VTT>>
    : concrete_type<base_set_type<VTT>, expand_types<VTT, set_of>> {};

template <typename KTT, typename VTT>
struct base_map_type : base_type<BaseType::Map> {
  using key_type = KTT;
  using mapped_type = VTT;
};

template <typename KTT, typename VTT, typename = void>
struct map_type : base_map_type<KTT, VTT> {};

template <typename KTT, typename VTT>
struct map_type<KTT, VTT, if_all_concrete<KTT, VTT>>
    : concrete_type<
          base_map_type<KTT, VTT>,
          expand_types<VTT, map_to<typename traits<KTT>::standard_type>>> {};

// Type traits for all primitive types.
template <>
struct traits<bool_t> : primitive_type<BaseType::Bool, bool> {};
template <>
struct traits<byte_t> : primitive_type<BaseType::Byte, int8_t, uint8_t> {};
template <>
struct traits<i16_t> : primitive_type<BaseType::I16, int16_t, uint16_t> {};
template <>
struct traits<i32_t> : primitive_type<BaseType::I32, int32_t, uint32_t> {};
template <>
struct traits<i64_t> : primitive_type<BaseType::I64, int64_t, uint64_t> {};
template <>
struct traits<float_t> : primitive_type<BaseType::Float, float> {};
template <>
struct traits<double_t> : primitive_type<BaseType::Double, double> {};
template <>
struct traits<string_t> : primitive_type<
                              BaseType::String,
                              std::string,
                              folly::fbstring,
                              folly::IOBuf,
                              std::unique_ptr<folly::IOBuf>> {};
template <>
struct traits<binary_t> : primitive_type<
                              BaseType::Binary,
                              std::string,
                              folly::fbstring,
                              folly::IOBuf,
                              std::unique_ptr<folly::IOBuf>> {};

// The enum class of types.
template <>
struct traits<enum_c> : base_type<BaseType::Enum> {};
template <typename E>
struct traits<enum_t<E>> : cpp_type<BaseType::Enum, E> {};

// The struct class of types.
template <>
struct traits<struct_c> : base_type<BaseType::Struct> {};
template <typename T>
struct traits<struct_t<T>> : cpp_type<BaseType::Struct, T> {};

// The union class of types.
template <>
struct traits<union_c> : base_type<BaseType::Union> {};
template <typename T>
struct traits<union_t<T>> : cpp_type<BaseType::Union, T> {};

// The exception class of types.
template <>
struct traits<exception_c> : base_type<BaseType::Exception> {};
template <typename T>
struct traits<exception_t<T>> : cpp_type<BaseType::Exception, T> {};

// The list class of types.
template <>
struct traits<list_c> : base_type<BaseType::List> {};
template <typename VTT>
struct traits<type::list<VTT>> : list_type<VTT> {};

// The set class of types.
template <>
struct traits<set_c> : base_type<BaseType::Set> {};
template <typename VTT>
struct traits<set<VTT>> : set_type<VTT> {};

// The map class of types.
template <>
struct traits<map_c> : base_type<BaseType::Map> {};
template <typename KTT, typename VTT>
struct traits<map<KTT, VTT>> : map_type<KTT, VTT> {};

// Helper to get human readable name for the type tag.
template <typename TT>
struct get_name_fn {
  // Return the name of the base type by default.
  FOLLY_EXPORT const std::string& operator()() const noexcept {
    static const auto* kName = new std::string([]() {
      std::string name;
      if (const char* cname =
              TEnumTraits<BaseType>::findName(traits<TT>::kBaseType)) {
        name = cname;
        folly::toLowerAscii(name);
      }
      return name;
    }());
    return *kName;
  }
};

// Helper for any 'named' types.
template <typename T, typename Module, typename Name>
struct get_name_named_fn {
  FOLLY_EXPORT const std::string& operator()() const noexcept {
    static const auto* kName = new std::string([]() {
      // TODO(afuller): Return thrift.uri if available.
      using info = reflect_module<Module>;
      return fmt::format(
          "{}.{}", fatal::z_data<typename info::name>(), fatal::z_data<Name>());
    }());
    return *kName;
  }
};

template <typename T>
struct get_name_fn<enum_t<T>> : get_name_named_fn<
                                    T,
                                    typename reflect_enum<T>::module,
                                    typename reflect_enum<T>::traits::name> {};

template <typename T>
struct get_name_fn<union_t<T>>
    : get_name_named_fn<
          T,
          typename reflect_variant<T>::module,
          typename reflect_variant<T>::traits::name> {};

template <typename T>
struct get_name_fn<struct_t<T>> : get_name_named_fn<
                                      T,
                                      typename reflect_struct<T>::module,
                                      typename reflect_struct<T>::name> {};

template <typename T>
struct get_name_fn<exception_t<T>> : get_name_named_fn<
                                         T,
                                         typename reflect_struct<T>::module,
                                         typename reflect_struct<T>::name> {};

template <typename VTT>
struct get_name_fn<list<VTT>> {
  FOLLY_EXPORT const std::string& operator()() const noexcept {
    static const auto* kName =
        new std::string(fmt::format("list<{}>", get_name_fn<VTT>()()));
    return *kName;
  }
};

template <typename VTT>
struct get_name_fn<set<VTT>> {
  FOLLY_EXPORT const std::string& operator()() const noexcept {
    static const auto* kName =
        new std::string(fmt::format("set<{}>", get_name_fn<VTT>()()));
    return *kName;
  }
};

template <typename KTT, typename VTT>
struct get_name_fn<map<KTT, VTT>> {
  FOLLY_EXPORT const std::string& operator()() const noexcept {
    static const auto* kName = new std::string(
        fmt::format("map<{}, {}>", get_name_fn<KTT>()(), get_name_fn<VTT>()()));
    return *kName;
  }
};

} // namespace apache::thrift::type::detail
