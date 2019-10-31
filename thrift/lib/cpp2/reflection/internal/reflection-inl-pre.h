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

#ifndef THRIFT_FATAL_REFLECTION_INL_PRE_H_
#define THRIFT_FATAL_REFLECTION_INL_PRE_H_ 1

#if !defined THRIFT_FATAL_REFLECTION_H_
#error "This file must be included from reflection.h"
#endif

namespace apache {
namespace thrift {
namespace detail {

template <typename Tag>
struct invoke_reffer_thru_or_access_field;

template <typename, typename, bool IsTry, typename Default = void>
struct reflect_module_tag_selector {
  using type = Default;
  static_assert(
      IsTry,
      "given type has no reflection metadata or is not a struct, enum or union");
};

template <typename>
struct reflect_module_tag_get;
template <typename, typename>
struct reflect_module_tag_try_get;
template <typename>
struct reflect_type_class_impl;

struct reflection_metadata_tag {};
struct struct_traits_metadata_tag {};

namespace reflection_impl {

template <typename, typename>
struct isset;

struct variant_member_name {
  template <typename Descriptor>
  using apply = typename Descriptor::metadata::name;
};

struct variant_member_field_id {
  template <typename Descriptor>
  using apply = typename Descriptor::metadata::id;
};

template <typename A>
struct data_member_accessor : private invoke_reffer_thru_or_access_field<A> {
 public:
  using invoke_reffer_thru_or_access_field<A>::operator();
};

template <typename... A>
struct chained_data_member_accessor;
template <>
struct chained_data_member_accessor<> {
  template <typename T>
  FOLLY_ERASE constexpr T&& operator()(T&& t) const noexcept {
    return static_cast<T&&>(t);
  }
};
template <typename V, typename... A>
struct chained_data_member_accessor<V, A...> {
  template <typename T>
  FOLLY_ERASE constexpr auto operator()(T&& t) const noexcept(
      noexcept(chained_data_member_accessor<A...>{}(V{}(static_cast<T&&>(t)))))
      -> decltype(
          chained_data_member_accessor<A...>{}(V{}(static_cast<T&&>(t)))) {
    return chained_data_member_accessor<A...>{}(V{}(static_cast<T&&>(t)));
  }
};

template <typename A>
struct invoker_adaptor {
  template <typename T>
  using has = folly::is_invocable<A, T>;
  template <typename T>
  using reference = folly::invoke_result_t<A, T>;
  template <typename T>
  using type = folly::remove_cvref_t<reference<T>>;
  template <typename T>
  FOLLY_ERASE static constexpr reference<T> ref(T&& t) {
    return A{}(static_cast<T&&>(t));
  }
  template <typename T>
  FOLLY_ERASE static constexpr type<T> copy(T&& t) {
    return A{}(static_cast<T&&>(t));
  }
};

template <typename G>
struct getter_direct_getter;
template <typename G>
struct getter_direct_getter<invoker_adaptor<G>> {
  using type = invoker_adaptor<G>;
};
template <typename V, typename... A>
struct getter_direct_getter<
    invoker_adaptor<chained_data_member_accessor<V, A...>>> {
  using type = invoker_adaptor<V>;
};
template <typename G>
using getter_direct_getter_t = folly::_t<getter_direct_getter<G>>;

} // namespace reflection_impl
} // namespace detail

#define THRIFT_REGISTER_REFLECTION_METADATA(Tag, ...)    \
  FATAL_REGISTER_TYPE(                                   \
      ::apache::thrift::detail::reflection_metadata_tag, \
      Tag,                                               \
      ::apache::thrift::reflected_module<__VA_ARGS__>)

#define THRIFT_REGISTER_STRUCT_TRAITS(Struct, ...)          \
  FATAL_REGISTER_TYPE(                                      \
      ::apache::thrift::detail::struct_traits_metadata_tag, \
      Struct,                                               \
      ::apache::thrift::reflected_struct<Struct, __VA_ARGS__>)

template <typename = void>
struct reflected_annotations;

} // namespace thrift
} // namespace apache

#endif // THRIFT_FATAL_REFLECTION_INL_PRE_H_
