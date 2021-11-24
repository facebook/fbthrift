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

#include <fmt/core.h>
#include <fatal/type/array.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/detail/Traits.h>
#include <thrift/lib/thrift/gen-cpp2/type_fatal.h>
#include <thrift/lib/thrift/gen-cpp2/type_types.h>

namespace apache::thrift::type::detail {

// Helper to get human readable name for the type tag.
template <typename Tag>
struct get_name_fn {
  // Return the name of the base type by default.
  FOLLY_EXPORT const std::string& operator()() const {
    static const auto* kName = new std::string([]() {
      std::string name;
      if (const char* cname =
              TEnumTraits<BaseType>::findName(traits<Tag>::kBaseType)) {
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
  FOLLY_EXPORT const std::string& operator()() const {
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

template <typename ValTag, template <typename...> typename ListT>
struct get_name_fn<list<ValTag, ListT>> {
  FOLLY_EXPORT const std::string& operator()() const {
    static const auto* kName =
        new std::string(fmt::format("list<{}>", get_name_fn<ValTag>()()));
    return *kName;
  }
};

template <typename KeyTag, template <typename...> typename SetT>
struct get_name_fn<set<KeyTag, SetT>> {
  FOLLY_EXPORT const std::string& operator()() const {
    static const auto* kName =
        new std::string(fmt::format("set<{}>", get_name_fn<KeyTag>()()));
    return *kName;
  }
};

template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
struct get_name_fn<map<KeyTag, ValTag, MapT>> {
  FOLLY_EXPORT const std::string& operator()() const {
    static const auto* kName = new std::string(fmt::format(
        "map<{}, {}>", get_name_fn<KeyTag>()(), get_name_fn<ValTag>()()));
    return *kName;
  }
};

template <typename Adapter, typename Tag>
struct get_name_fn<adapted<Adapter, Tag>> {
  FOLLY_EXPORT const std::string& operator()() const {
    static const auto* kName =
        new std::string(folly::pretty_name<
                        typename traits<adapted<Adapter, Tag>>::native_type>());
    return *kName;
  }
};

} // namespace apache::thrift::type::detail
