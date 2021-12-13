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
#include <fatal/type/slice.h>
#include <fatal/type/transform.h>
#include <folly/String.h>
#include <folly/Traits.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/Adapt.h>
#include <thrift/lib/cpp2/type/AnyType.h>
#include <thrift/lib/cpp2/type/BaseType.h>
#include <thrift/lib/cpp2/type/ThriftType.h>

namespace apache::thrift::type::detail {

// All the traits for the given tag.
template <typename Tag>
struct traits {
  // No types to declare for non concrete types.
  static_assert(is_not_concrete_v<Tag>);
};

// A 'bound' helper for resolving concrete template types.
template <template <typename...> typename TemplateT, typename... Front>
struct template_of {
  template <typename T>
  using apply = TemplateT<Front..., T>;
};

// Resolves the concrete template type when paramaterizing the given template,
// with the native types of the give Tags.
template <template <typename...> typename TemplateT, typename... Tags>
using native_template_t = TemplateT<typename traits<Tags>::native_type...>;
template <template <typename...> typename TemplateT, typename... Tags>
struct native_template {
  using type = native_template_t<TemplateT, Tags...>;
};

template <typename... Tags>
using if_all_concrete = std::enable_if_t<(is_concrete_v<Tags> && ...)>;

template <typename... Tags>
struct types {
  static constexpr bool contains(BaseType baseType) {
    return (... || (base_type_v<Tags> == baseType));
  }

  template <typename Tag>
  static constexpr bool contains() {
    return contains(base_type_v<Tag>);
  }

  static bool contains(const AnyType& type) {
    return contains(type.base_type());
  }

  // The Ith type.
  template <size_t I>
  using at = typename fatal::at<types, I>;

  // Converts the type list to a type list of the given types.
  template <template <typename...> typename T>
  using as = T<Tags...>;
};

template <typename Ts, typename Tag, typename R = void>
using if_contains = std::enable_if_t<Ts::template contains<Tag>(), R>;

template <typename Tag, typename A>
using expand_types = fatal::transform<typename traits<Tag>::standard_types, A>;

template <typename A, typename... Tags>
using apply_to_native_types =
    typename A::template apply<typename traits<Tags>::native_type...>;

// The (mixin) traits all concrete types (_t suffix) define.
//
// All concrete types have an associated set of standard types and a native type
// (which by default is just the first standard type).
template <typename StandardTs, typename NativeType = fatal::first<StandardTs>>
struct concrete_type {
  using standard_types = StandardTs;
  using native_type = NativeType;
  using standard_type = fatal::first<standard_types>;
};

// The traits for a concrete type that is associated with specific c++
// types.
template <typename... StandardTs>
using cpp_type = concrete_type<types<StandardTs...>>;

template <>
struct traits<void_t> : cpp_type<void> {};

// Type traits for all primitive types.
template <>
struct traits<bool_t> : cpp_type<bool> {};
template <>
struct traits<byte_t> : cpp_type<int8_t, uint8_t> {};
template <>
struct traits<i16_t> : cpp_type<int16_t, uint16_t> {};
template <>
struct traits<i32_t> : cpp_type<int32_t, uint32_t> {};
template <>
struct traits<i64_t> : cpp_type<int64_t, uint64_t> {};
template <>
struct traits<float_t> : cpp_type<float> {};
template <>
struct traits<double_t> : cpp_type<double> {};
template <>
struct traits<string_t> : cpp_type<
                              std::string,
                              folly::fbstring,
                              folly::IOBuf,
                              std::unique_ptr<folly::IOBuf>> {};
template <>
struct traits<binary_t> : cpp_type<
                              std::string,
                              folly::fbstring,
                              folly::IOBuf,
                              std::unique_ptr<folly::IOBuf>> {};

// Traits for enums.
template <typename E>
struct traits<enum_t<E>> : cpp_type<E> {};

// Traits for structs.
template <typename T>
struct traits<struct_t<T>> : cpp_type<T> {};

// Traits for unions.
template <typename T>
struct traits<union_t<T>> : cpp_type<T> {};

// Traits for excpetions.
template <typename T>
struct traits<exception_t<T>> : cpp_type<T> {};

// The non-concrete list traits.
template <
    typename ValTag,
    template <typename...>
    typename ListT,
    typename = void>
struct list_type {
  using value_type = ValTag;
};

// The concrete list traits.
template <typename ValTag, template <typename...> typename ListT>
struct list_type<ValTag, ListT, if_all_concrete<ValTag>>
    : concrete_type<
          expand_types<ValTag, template_of<ListT>>,
          native_template_t<ListT, ValTag>> {
  using value_type = ValTag;
};

// Traits for lists.
template <typename ValTag>
struct traits<type::list<ValTag, DefaultT>>
    : traits<type::list<ValTag, std::vector>> {};
template <typename ValTag, template <typename...> typename ListT>
struct traits<type::list<ValTag, ListT>> : list_type<ValTag, ListT> {};

// Helpers replace less, hash, equal_to functions
// for a set, with the appropriate adapted versions.
template <
    typename Adapter,
    template <typename, typename, typename>
    typename SetT,
    typename Key,
    typename Less,
    typename Allocator>
SetT<Key, adapt_detail::adapted_less<Adapter, Key>, Allocator>
resolveSetForAdapated(const SetT<Key, Less, Allocator>&);
template <
    typename Adapter,
    template <typename, typename, typename, typename>
    typename SetT,
    typename Key,
    typename Hash,
    typename KeyEqual,
    typename Allocator>
SetT<
    Key,
    adapt_detail::adapted_hash<Adapter, Key>,
    adapt_detail::adapted_equal<Adapter, Key>,
    Allocator>
resolveSetForAdapated(const SetT<Key, Hash, KeyEqual, Allocator>&);

// Normal element types just use the default template parameters.
template <template <typename...> typename SetT, typename KeyTag>
struct native_set : native_template<SetT, KeyTag> {};

// Adapted elements use adapted template arguments.
template <
    template <typename...>
    typename SetT,
    typename Adapter,
    typename KeyTag>
struct native_set<SetT, adapted<Adapter, KeyTag>> {
  using type = decltype(resolveSetForAdapated<Adapter>(
      std::declval<native_template_t<SetT, adapted<Adapter, KeyTag>>>()));
};

// The non-concrete set traits.
template <
    typename KeyTag,
    template <typename...>
    typename SetT,
    typename = void>
struct set_type {
  using key_type = KeyTag;
};

// The concrete set traits.
template <typename KeyTag, template <typename...> typename SetT>
struct set_type<KeyTag, SetT, if_all_concrete<KeyTag>>
    : concrete_type<
          expand_types<KeyTag, template_of<SetT>>,
          typename native_set<SetT, KeyTag>::type> {
  using key_type = KeyTag;
};

// Traits for sets.
template <typename KeyTag>
struct traits<set<KeyTag, DefaultT>> : set_type<KeyTag, std::set> {};
template <typename KeyTag, template <typename...> typename SetT>
struct traits<set<KeyTag, SetT>> : set_type<KeyTag, SetT> {};

// Helpers to set the appropriate less, hash, equal_to functions
// for a map with an adapted key type.
template <
    typename Adapter,
    template <typename, typename, typename, typename>
    typename MapT,
    typename Key,
    typename Value,
    typename Less,
    typename Allocator>
MapT<Key, Value, adapt_detail::adapted_less<Adapter, Key>, Allocator>
resolveMapForAdapated(const MapT<Key, Value, Less, Allocator>&);
template <
    typename Adapter,
    template <typename, typename, typename, typename, typename>
    typename MapT,
    typename Key,
    typename Value,
    typename Hash,
    typename KeyEqual,
    typename Allocator>
MapT<
    Key,
    Value,
    adapt_detail::adapted_hash<Adapter, Key>,
    adapt_detail::adapted_equal<Adapter, Key>,
    Allocator>
resolveMapForAdapated(const MapT<Key, Value, Hash, KeyEqual, Allocator>&);
template <
    template <typename...>
    typename MapT,
    typename KeyTag,
    typename ValTag>
struct native_map : native_template<MapT, KeyTag, ValTag> {};
template <
    typename Adapter,
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
struct native_map<MapT, adapted<Adapter, KeyTag>, ValTag> {
  using type = decltype(resolveMapForAdapated<Adapter>(
      std::declval<
          native_template_t<MapT, adapted<Adapter, KeyTag>, ValTag>>()));
};

// The non-concrete map traits.
template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT,
    typename = void>
struct map_type {
  using key_type = KeyTag;
  using value_type = ValTag;
};

// The concrete map traits.
template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
struct map_type<KeyTag, ValTag, MapT, if_all_concrete<KeyTag, ValTag>>
    : concrete_type<
          expand_types<
              ValTag,
              template_of<MapT, typename traits<KeyTag>::standard_type>>,
          typename native_map<MapT, KeyTag, ValTag>::type> {
  using key_type = KeyTag;
  using value_type = ValTag;
};

// Traits for maps.
template <typename KeyTag, typename ValTag>
struct traits<map<KeyTag, ValTag, DefaultT>>
    : traits<map<KeyTag, ValTag, std::map>> {};
template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
struct traits<map<KeyTag, ValTag, MapT>> : map_type<KeyTag, ValTag, MapT> {};

// Traits for adapted types.
//
// Adapted types are concrete and have an adapted native_type.
template <typename Adapter, typename Tag>
struct traits<adapted<Adapter, Tag>>
    : concrete_type<
          typename traits<Tag>::standard_types,
          adapt_detail::adapted_t<Adapter, typename traits<Tag>::native_type>> {
};

} // namespace apache::thrift::type::detail
