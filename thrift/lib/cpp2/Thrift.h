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

#ifndef THRIFT_CPP2_H_
#define THRIFT_CPP2_H_

#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/protocol/TType.h>
#include <thrift/lib/cpp2/FieldRef.h>
#include <thrift/lib/cpp2/OptionalField.h>
#include <thrift/lib/cpp2/TypeClass.h>

#include <folly/Traits.h>
#include <initializer_list>
#include <utility>

#include <cstdint>

namespace apache {
namespace thrift {

enum FragileConstructor {
  FRAGILE,
};

namespace detail {
namespace st {

//  struct_private_access
//
//  Thrift structures have private members but it may be necessary for the
//  Thrift support library to access those private members.
struct struct_private_access {
  //  These should be alias templates but Clang has a bug where it does not
  //  permit member alias templates of a friend struct to access private
  //  members of the type to which it is a friend. Making these function
  //  templates is a workaround.
  template <typename T>
  static folly::bool_constant<T::__fbthrift_cpp2_gen_json> //
  __fbthrift_cpp2_gen_json();
  template <typename T>
  static folly::bool_constant<T::__fbthrift_cpp2_gen_nimble> //
  __fbthrift_cpp2_gen_nimble();
};

template <typename T, typename = void>
struct IsThriftClass : std::false_type {};

template <typename T>
struct IsThriftClass<T, folly::void_t<typename T::__fbthrift_cpp2_type>>
    : std::true_type {};

template <typename T, typename = void>
struct IsThriftUnion : std::false_type {};

template <typename T>
struct IsThriftUnion<T, folly::void_t<typename T::__fbthrift_cpp2_type>>
    : folly::bool_constant<T::__fbthrift_cpp2_is_union> {};

} // namespace st
} // namespace detail

namespace detail {

template <typename T>
struct enum_hash {
  size_t operator()(T t) const {
    using underlying_t = typename std::underlying_type<T>::type;
    return std::hash<underlying_t>()(underlying_t(t));
  }
};

template <typename T>
struct enum_equal_to {
  bool operator()(T t0, T t1) const {
    return t0 == t1;
  }
};

} // namespace detail

namespace detail {

// Adapted from Fatal (https://github.com/facebook/fatal/)
// Inlined here to keep the amount of mandatory dependencies at bay
// For more context, see http://ericniebler.com/2013/08/07/
// - Universal References and the Copy Constructor
template <typename, typename...>
struct is_safe_overload {
  using type = std::true_type;
};
template <typename Class, typename T>
struct is_safe_overload<Class, T> {
  using type = std::integral_constant<
      bool,
      !std::is_same<
          Class,
          typename std::remove_cv<
              typename std::remove_reference<T>::type>::type>::value>;
};

} // namespace detail

template <typename Class, typename... Args>
using safe_overload_t = typename std::enable_if<
    detail::is_safe_overload<Class, Args...>::type::value>::type;

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_CPP2_H_
