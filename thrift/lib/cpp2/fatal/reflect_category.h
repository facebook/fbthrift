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

#include <fatal/type/enum.h>
#include <fatal/type/variant_traits.h>

#include <type_traits>

namespace apache { namespace thrift {
namespace detail {
template <typename> struct reflect_category_impl;
} // namespace detail {

/**
 * Tells whether the given type is a tag that represents the reflection metadata
 * of the types declared in a Thrift file.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace My.Namespace
 *
 *  struct MyStruct {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 *  /////////////
 *  // foo.cpp //
 *  /////////////
 *
 *  // yields `std::true_type`
 *  using result1 = is_reflectable_module<MyModule_tags::metadata>;
 *
 *  // yields `std::false_type`
 *  using result2 = is_reflectable_module<MyStruct>;
 *
 *  // yields `std::false_type`
 *  using result3 = is_reflectable_module<void>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
using is_reflectable_module = std::integral_constant<
  bool,
  !std::is_same<
    try_reflect_module<T, void>,
    void
  >::value
>;

/**
 * Tells whether the given type is a Thrift struct with compile-time reflection
 * support.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace My.Namespace
 *
 *  struct MyStruct {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 *  enum MyEnum { a, b, c }
 *
 *  /////////////
 *  // foo.cpp //
 *  /////////////
 *
 *  // yields `std::true_type`
 *  using result1 = is_reflectable_struct<MyStruct>;
 *
 *  // yields `std::false_type`
 *  using result2 = is_reflectable_struct<MyEnum>;
 *
 *  // yields `std::false_type`
 *  using result3 = is_reflectable_struct<void>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
using is_reflectable_struct = std::integral_constant<
  bool,
  !std::is_same<
    try_reflect_struct<T, void>,
    void
  >::value
>;

/**
 * Tells whether the given type is a Thrift enum with compile-time reflection
 * support.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace My.Namespace
 *
 *  union MyUnion {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 *  enum MyEnum { a, b, c }
 *
 *  /////////////
 *  // foo.cpp //
 *  /////////////
 *
 *  // yields `std::true_type`
 *  using result1 = is_reflectable_enum<MyEnum>;
 *
 *  // yields `std::false_type`
 *  using result2 = is_reflectable_enum<MyUnion>;
 *
 *  // yields `std::false_type`
 *  using result3 = is_reflectable_enum<void>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
using is_reflectable_enum = fatal::has_enum_traits<T>;

/**
 * Tells whether the given type is a Thrift union with compile-time reflection
 * support.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace My.Namespace
 *
 *  union MyUnion {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 *  enum MyEnum { a, b, c }
 *
 *  /////////////
 *  // foo.cpp //
 *  /////////////
 *
 *  // yields `std::true_type`
 *  using result1 = is_reflectable_union<MyUnion>;
 *
 *  // yields `std::false_type`
 *  using result2 = is_reflectable_union<MyEnum>;
 *
 *  // yields `std::false_type`
 *  using result3 = is_reflectable_union<void>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
using is_reflectable_union = fatal::has_variant_traits<T>;

/**
 * Returns the Thrift category of a type.
 *
 * See `thrift_category` for the possible categories.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace My.Namespace
 *
 *  struct MyStruct {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 *  enum MyEnum { a, b, c }
 *
 *  typedef list<string> MyList;
 *
 *  /////////////
 *  // foo.cpp //
 *  /////////////
 *
 *  // yields `std::integral_constant<
 *  //   thrift_category,
 *  //   thrift_category::structs
 *  // >`
 *  using result1 = reflect_category<MyStruct>;
 *
 *  // yields `std::integral_constant<thrift_category, thrift_category::enums>`
 *  using result2 = reflect_category<MyEnum>;
 *
 *  // yields `std::integral_constant<thrift_category, thrift_category::lists>`
 *  using result3 = reflect_category<MyList>;
 *
 *  // yields `std::integral_constant<
 *  //   thrift_category,
 *  //   thrift_category::unknown
 *  // >`
 *  using result4 = reflect_category<void>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
using reflect_category = typename detail::reflect_category_impl<T>::type;

/**
 * This template is used internally by `reflect_category` as a fallback to
 * figure out categories for unknown types.
 *
 * Specialize this template in order to make `reflect_category` support
 * additional types or templates.
 *
 * Note that `is_reflectable_module`, `is_reflectable_struct`,
 * `is_reflectable_enum` and `is_reflectable_union` will be unaffected.
 *
 * Example:
 *
 *  template <typename K, typename V>
 *  struct get_thrift_category<my_custom_map<K, V>> {
 *    using type = std::integral_constant<
 *      thrift_category,
 *      thrift_category::maps
 *    >;
 *  };
 *
 *  // yields `std::integral_constant<thrift_category, thrift_category::maps>`
 *  using result1 = reflect_category<my_custom_map<int, std::string>>;
 *
 *  // yields `std::integral_constant<thrift_category, thrift_category::maps>`
 *  using result2 = reflect_category<my_custom_map<int, MyStruct>>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename>
struct get_thrift_category{
  using type = std::integral_constant<
    thrift_category,
    thrift_category::unknown
  >;
};

}} // apache::thrift

#include <thrift/lib/cpp2/fatal/reflect_category-inl.h>

#endif // THRIFT_FATAL_REFLECT_CATEGORY_H_
