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
#ifndef THRIFT_FATAL_REFLECTION_H_
#define THRIFT_FATAL_REFLECTION_H_ 1

#include <thrift/lib/cpp2/fatal/reflection-inl-pre.h>

#include <fatal/type/enum.h>
#include <fatal/type/map.h>
#include <fatal/type/registry.h>
#include <fatal/type/transform.h>
#include <fatal/type/variant_traits.h>

#include <type_traits>

#include <cstdint>

namespace apache { namespace thrift {

/**
 * An alias to the type used by Thrift as a struct's field ID.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
using field_id_t = std::int16_t;

/**
 * The high-level category of a type as it concerns Thrift.
 *
 * See `reflect_category` for more information.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
enum class thrift_category {
  /**
   * Represents types unknown to the reflection framework.
   *
   * This often means it's a custom type with no `get_thrift_category`
   * specialization.
   *
   * See documentation for `get_thrift_category`.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  unknown,
  /**
   * Represents types with no actual data representation. Most commonly `void`.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  nothing,
  /**
   * Represents all signed and unsigned integral types, including `bool`.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  integral,
  /**
   * Represents all floating point types.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  floating_point,
  /**
   * Represents all known strings implementation.
   *
   * If this is not the category returned for a string, this often means there's
   * no `get_thrift_category` specialization for it.
   *
   * See documentation for `get_thrift_category`.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  string,
  /**
   * Represents an enum.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  enumeration,
  /**
   * Represents an class or structure.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  structure,
  /**
   * Represents a variant (or union, as the Thrift IDL grammar goes).
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  variant,
  /**
   * Represents all known list implementations.
   *
   * If this is not the category returned for a list, this often means there's
   * no `get_thrift_category` specialization for it.
   *
   * See documentation for `get_thrift_category`.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  list,
  /**
   * Represents all known set implementations.
   *
   * If this is not the category returned for a set, this often means there's
   * no `get_thrift_category` specialization for it.
   *
   * See documentation for `get_thrift_category`.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  set,
  /**
   * Represents all known map implementations.
   *
   * If this is not the category returned for a map, this often means there's
   * no `get_thrift_category` specialization for it.
   *
   * See documentation for `get_thrift_category`.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  map
};

/**
 * Holds reflection metadata for stuff defined in a Thrift file.
 *
 * NOTE: this class template is only intended to be instantiated by Thrift.
 * Users should ignore the template parameters taken by it and focus simply on
 * the members provided.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <
  typename Namespaces,
  typename Enums,
  typename Unions,
  typename Structs,
  typename Constants,
  typename Services
>
struct reflected_module {
  /**
   * The map from language to namespace.
   *
   * A `fatal::type_map` where the key is a `fatal::constant_sequence`
   * (compile-time string) representing the language, associated with a
   * compile-time string representing the namespace.
   *
   * Example:
   *
   *  // MyModule.thrift
   *
   *  namespace cpp My.NamespaceCpp
   *  namespace cpp2 My.Namespace
   *  namespace java My.Namespace
   *
   *  struct MyStruct {
   *    1: i32 a
   *    2: string b
   *    3: double c
   *  }
   *
   *  // C++
   *
   *  using info = reflect_module<My::Namespace::MyModule_tags::metadata>;
   *
   *  FATAL_STR(cpp, "cpp");
   *  FATAL_STR(cpp2, "cpp2");
   *  FATAL_STR(java, "java");
   *
   *  // yields `fatal::constant_sequence<
   *  //   char,
   *  //   'M', 'y', ':', ':', 'N', 'a', 'm', 'e',
   *  //   's', 'p', 'a', 'c', 'e', 'C', 'p', 'p'
   *  // >`
   *  using result1 = info::namespaces::get<cpp>;
   *
   *  // yields `fatal::constant_sequence<
   *  //   char, 'M', 'y', ':', ':', 'N', 'a', 'm', 'e', 's', 'p', 'a', 'c', 'e'
   *  // >`
   *  using result2 = info::namespaces::get<cpp2>;
   *
   *  // yields `fatal::constant_sequence<
   *  //   char, 'M', 'y', '.', 'N', 'a', 'm', 'e', 's', 'p', 'a', 'c', 'e'
   *  // >`
   *  using result3 = info::namespaces::get<java>;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using namespaces = Namespaces;

  /**
   * The list of enumerations with reflection metadata available.
   *
   * A `fatal::type_map` where the key is the type generated by the Thrift
   * compiler for each enumeration, associated with a compile-time string
   * (`fatal::constant_sequence`) representing the enumeration name.
   *
   * Use `fatal::enum_traits` to retrieve reflection information for each
   * enumeration (fatal/type/enum.h).
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using enums = Enums;

  /**
   * The list of unions with reflection metadata available.
   *
   * A `fatal::type_map` where the key is the type generated by the Thrift
   * compiler for each union, associated with a compile-time string
   * (`fatal::constant_sequence`) representing the union name.
   *
   * Use `fatal::variant_traits` to retrieve reflection information for each
   * union (fatal/type/variant_traits.h).
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using unions = Unions;

  /**
   * The list of structs with reflection metadata available.
   *
   * A `fatal::type_map` where the key is the type generated by the Thrift
   * compiler for each struct, associated with a compile-time string
   * (`fatal::constant_sequence`) representing the struct name.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using structs = Structs;

  /**
   * The list of services with reflection metadata available.
   *
   * A `fatal::type_list` of compile-time strings
   * (`fatal::constant_sequence`) representing each service name.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using services = Services;
};

/**
 * Holds reflection metadata for a given struct.
 *
 * NOTE: this class template is only intended to be instantiated by Thrift.
 * Users should ignore the template parameters taken by it and focus simply on
 * the members provided.
 *
 * For the examples below, consider code generated for this Thrift file:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace cpp2 My.Namespace
 *
 *  struct MyStruct {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <
  typename Struct,
  typename Name,
  typename Module,
  typename Names,
  typename Info
>
struct reflected_struct {
  /**
   * A type alias for the struct itself.
   *
   * Example:
   *
   *  using info = reflect_struct<MyStruct>;
   *
   *  // yields `MyStruct`
   *  using result = info::type;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using type = Struct;

  /**
   * A compile-time string representing the struct name.
   *
   * Example:
   *
   *  using info = reflect_struct<MyStruct>;
   *
   *  // yields `fatal::constant_sequence<
   *  //   char,
   *  //   'M', 'y', 'S', 't', 'r', 'u', 'c', 't'
   *  // >`
   *  using result = info::type;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using name = Name;

  /**
   * The reflection metadata tag for the Thrift file where this structure is
   * declared.
   *
   * Example:
   *
   *  using info = reflect_struct<MyStruct>;
   *
   *  // yields `My::Namespace::MyModule_tags::metadata`
   *  using result = info::module;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using module = Module;

  /**
   * An implementation defined type that provides the names for each data member
   * as a member type alias with the same name.
   *
   * These type aliases are used as the key for the various type maps offered by
   * the `reflected_struct` class.
   *
   * The names are represented by a `fatal::constant_sequence` of type `char`
   * (a compile-time string).
   *
   * Example:
   *
   *  using info = reflect_struct<MyStruct>;
   *
   *  // yields `fatal::constant_sequence<char, 'a'>`
   *  using result1 = info::names::a;
   *
   *  // yields `fatal::constant_sequence<char, 'b'>`
   *  using result2 = info::names::b;
   *
   *  // yields "c"
   *  auto result3 = info::names::c::z_data();
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using names = Names;

  /**
   * A `fatal::type_map` from the data member name to the corresponding metadata
   * for that data member as a `reflected_struct_data_member`.
   *
   * See the documentation for `reflected_struct_data_member` (below) for more
   * information on its members.
   *
   * Example:
   *
   *  using info = reflect_struct<MyStruct>;
   *
   *  using member = info::members<info::names::b>;
   *
   *  // yields "b"
   *  auto result1 = member::name::z_data();
   *
   *  // yields `std::string`
   *  using result2 = member::type;
   *
   *  // yields `2`
   *  auto result3 = member::id::value;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using members = typename fatal::type_map_from<
    fatal::get_member_type::name
  >::template list<Info>;

  /**
   * A `fatal::type_map` from the data member name to the data member type.
   *
   * Example:
   *
   *  using info = reflect_struct<MyStruct>;
   *
   *  // yields `double`
   *  using result = info::types::get<info::names::c>;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using types = typename members::template transform<
    fatal::get_member_type::type
  >;

  /**
   * A `fatal::type_map` from the data member name to the Thrift field id as a
   * `std::integral_constant` of type `field_id_t`.
   *
   * Example:
   *
   *  using info = reflect_struct<MyStruct>;
   *
   *  // yields `std::integral_constant<field_id_t, 2>`
   *  using result1 = info::ids::get<info::names::b>;
   *
   *  // yields `2`
   *  auto result2 = result1::value;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using ids = typename members::template transform<
    fatal::get_member_type::id
  >;

  /**
   * A `fatal::type_map` from the data member name to a getter for the data
   * member.
   *
   * See the documentation for `reflected_struct_data_member::getter` (below)
   * for more information.
   *
   * Example:
   *
   *  using info = reflect_struct<MyStruct>;
   *
   *  using getter = info::getters::get<info::names::a>;
   *
   *  MyStruct pod;
   *
   *  pod.a = 10;
   *
   *  // yields `10`
   *  auto result1 = getter::ref(pod);
   *
   *  // sets  `56` on `pod.a`
   *  getter::ref(pod) = 56;
   *
   *  // yields `56`
   *  auto result2 = pod.a;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using getters = typename members::template transform<
    fatal::get_member_type::getter
  >;
};

/**
 * Holds reflection metadata for a given struct's data member.
 *
 * NOTE: this class template is only intended to be instantiated by Thrift.
 * Users should ignore the template parameters taken by it and focus simply on
 * the members provided.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <
  typename Name,
  typename Type,
  field_id_t Id,
  typename Getter,
  thrift_category Category,
  template <typename> class Pod
>
struct reflected_struct_data_member {
  /**
   * A `fatal::constant_sequence` of `char` representing the data member name as
   * a compile-time string.
   *
   * Example:
   *
   *  // MyModule.thrift
   *
   *  namespace cpp2 My.Namespace
   *
   *  struct MyStruct {
   *    1: i32 fieldA
   *    2: string fieldB
   *    3: double fieldC
   *  }
   *
   *  // C++
   *
   *  using info = reflect_struct<MyStruct>;
   *  using member = info::types::members<info::names::fieldC>;
   *
   *  // yields `fatal::constant_sequence<char, 'f', 'i', 'e', 'l', 'd', 'C'>`
   *  using result1 = member::name;
   *
   *  // yields "fieldC"
   *  auto result2 = result1::z_data();
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using name = Name;

  /**
   * The type of the data member.
   *
   * Example:
   *
   *  // MyModule.thrift
   *
   *  namespace cpp2 My.Namespace
   *
   *  struct MyStruct {
   *    1: i32 fieldA
   *    2: string fieldB
   *    3: double fieldC
   *  }
   *
   *  // C++
   *
   *  using info = reflect_struct<MyStruct>;
   *  using member = info::types::members<info::names::fieldC>;
   *
   *  // yields `double`
   *  using result1 = member::type;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using type = Type;

  /**
   * A `std::integral_constant` of type `field_id_t` representing the Thrift
   * field id for the data member.
   *
   * Example:
   *
   *  // MyModule.thrift
   *
   *  namespace cpp2 My.Namespace
   *
   *  struct MyStruct {
   *    1: i32 fieldA
   *    2: string fieldB
   *    3: double fieldC
   *  }
   *
   *  // C++
   *
   *  using info = reflect_struct<MyStruct>;
   *  using member = info::types::members<info::names::fieldC>;
   *
   *  // yields `std::integral_constant<field_id_t, 3>`
   *  using result1 = member::type;
   *
   *  // yields `3`
   *  auto result2 = result1::value;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using id = std::integral_constant<field_id_t, Id>;

  /**
   * A type that works as a getter for the data member.
   *
   * See also Fatal's documentation on `FATAL_DATA_MEMBER_GETTER` in
   * `fatal/type/traits.h` for more information on how to make the most out of
   * the data member getter.
   *
   * Example:
   *
   *  // MyModule.thrift
   *
   *  namespace cpp2 My.Namespace
   *
   *  struct MyStruct {
   *    1: i32 fieldA
   *    2: string fieldB
   *    3: double fieldC
   *  }
   *
   *  // C++
   *
   *  using info = reflect_struct<MyStruct>;
   *  using member = info::types::members<info::names::fieldC>;
   *
   *  using getter = info::getters::get<info::names::a>;
   *
   *  MyStruct pod;
   *
   *  pod.c = 7.2;
   *
   *  // yields `7.2`
   *  auto result1 = getter::ref(pod);
   *
   *  // sets  `5.6` on `pod.c`
   *  getter::ref(pod) = 5.6;
   *
   *  // yields `5.6`
   *  auto result2 = pod.c;
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using getter = Getter;

  /**
   * Tells what's the Thrift category for this member.
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  using category = std::integral_constant<thrift_category, Category>;

  /**
   * A POD (plain old data) that holds a single data member with the same name
   * as this member. The template parameter `T` defines the type of this POD's
   * sole member, and defaults to `type`.
   *
   * This is useful when you need to create a class with an API compatible to
   * the original Thrift struct.
   *
   * Example:
   *
   *  // MyModule.thrift
   *
   *  namespace cpp2 My.Namespace
   *
   *  struct MyStruct {
   *    1: i32 fieldA
   *    2: string fieldB
   *    3: double fieldC
   *  }
   *
   *  // C++
   *
   *  using info = reflect_struct<MyStruct>;
   *  using member = info::types::members<info::names::fieldC>;
   *
   *  member::pod<> original;
   *
   *  // yields `double`
   *  using result1 = decltype(original.fieldC);
   *
   *  member::pod<bool> another;
   *
   *  // yields `bool`
   *  using result2 = decltype(another.fieldC);
   *
   * @author: Marcelo Juchem <marcelo@fb.com>
   */
  template <typename T = type>
  using pod = Pod<T>;
};

/**
 * Retrieves reflection metadata (as a `reflected_module`) associated with the
 * given reflection metadata tag.
 *
 * The Thrift compiler generates a reflection metadata tag for each Thrift file
 * named `namespace::thriftfilename_tags::metadata`.
 *
 * If the given tag does not represent a Thrift module, or if there's no
 * reflection metadata available for it, compilation will fail.
 *
 * See the documentation on `reflected_module` (above) for more information on
 * the returned type.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace cpp2 My.Namespace
 *
 *  enum MyEnum1 { a, b, c }
 *  enum MyEnum2 { x, y, x }
 *
 *  //////////////////
 *  // whatever.cpp //
 *  //////////////////
 *  using info = reflect_module<My::Namespace::MyModule_tags::metadata>;
 *
 *  // yields `2`
 *  auto result1 = info::enums::size;
 *
 *  // fails compilation
 *  using result2 = reflect_module<void>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename Tag>
using reflect_module = fatal::registry_lookup<
  detail::reflection_impl::reflection_metadata_tag,
  Tag
>;

/**
 * Retrieves reflection metadata (as a `reflected_module`) associated with the
 * given reflection metadata tag.
 *
 * The Thrift compiler generates a reflection metadata tag for each Thrift file
 * named `namespace::thriftfilename_tags::metadata`.
 *
 * If the given tag does not represent a Thrift module, or if there's no
 * reflection metadata available for it, `Default` will be returned.
 *
 * See the documentation on `reflected_module` (above) for more information on
 * the returned type.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace cpp2 My.Namespace
 *
 *  enum MyEnum1 { a, b, c }
 *  enum MyEnum2 { x, y, x }
 *
 *  //////////////////
 *  // whatever.cpp //
 *  //////////////////
 *  using info = try_reflect_module<
 *    My::Namespace::MyModule_tags::metadata,
 *    void
 *  >;
 *
 *  // yields `2`
 *  auto result1 = info::enums::size;
 *
 *  // yields `void`
 *  using result2 = itry_reflect_module<int, void>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename Tag, typename Default>
using try_reflect_module = fatal::try_registry_lookup<
  detail::reflection_impl::reflection_metadata_tag,
  Tag,
  Default
>;

/**
 * Retrieves reflection metadata (as a `reflected_struct`) associated with the
 * given struct.
 *
 * If the given type is not a Thrift struct, or if there's no reflection
 * metadata available for it, compilation will fail.
 *
 * See the documentation on `reflected_struct` (above) for more information on
 * the returned type.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace cpp2 My.Namespace
 *
 *  struct MyStruct {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 *  //////////////////
 *  // whatever.cpp //
 *  //////////////////
 *  using info = reflect_struct<My::Namespace::MyStruct>;
 *
 *  // yields `3`
 *  auto result = info::members::size;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename Struct>
using reflect_struct = fatal::registry_lookup<
  detail::reflection_impl::struct_traits_metadata_tag,
  Struct
>;

/**
 * Retrieves reflection metadata (as a `reflected_struct`) associated with the
 * given struct.
 *
 * If the given type is not a Thrift struct, or if there's no reflection
 * metadata available for it, `Default` will be returned.
 *
 * See the documentation on `reflected_struct` (above) for more information on
 * the returned type.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace cpp2 My.Namespace
 *
 *  struct MyStruct {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 *  //////////////////
 *  // whatever.cpp //
 *  //////////////////
 *  using info = reflect_struct<My::Namespace::MyStruct>;
 *
 *  // yields `3`
 *  auto result = info::members::size;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename Struct, typename Default>
using try_reflect_struct = fatal::try_registry_lookup<
  detail::reflection_impl::struct_traits_metadata_tag,
  Struct,
  Default
>;

/**
 * Tells whether the given type is a tag that represents the reflection metadata
 * of the types declared in a Thrift file.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace cpp2 My.Namespace
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
 *  namespace cpp2 My.Namespace
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
 *  namespace cpp2 My.Namespace
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
 *  namespace cpp2 My.Namespace
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

namespace detail {
template <typename> struct reflect_category_impl;
} // namespace detail {

/**
 * Returns the Thrift category of a type.
 *
 * To keep compilation times at bay, strings and containers are not detected by
 * default, therefore they will yield `unknown` as the category. To enable their
 * detection you must include `reflect_category.h`.
 *
 * See `thrift_category` for the possible categories.
 *
 * Example:
 *
 *  /////////////////////
 *  // MyModule.thrift //
 *  /////////////////////
 *  namespace cpp2 My.Namespace
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
 *  //   thrift_category::structure
 *  // >`
 *  using result1 = reflect_category<MyStruct>;
 *
 *  // yields `std::integral_constant<
 *  //   thrift_category,
 *  //   thrift_category::enumeration
 *  // >`
 *  using result2 = reflect_category<MyEnum>;
 *
 *  // yields `std::integral_constant<
 *  //   thrift_category,
 *  //   thrift_category::list
 *  // >`
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
 *      thrift_category::map
 *    >;
 *  };
 *
 *  // yields `std::integral_constant<thrift_category, thrift_category::map>`
 *  using result1 = reflect_category<my_custom_map<int, std::string>>;
 *
 *  // yields `std::integral_constant<thrift_category, thrift_category::map>`
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

namespace detail {
template <typename> struct reflect_module_tag_impl;
} // namespace detail {

/**
 * Gets the reflection metadata tag for the Thrift file where the type `T` is
 * declared.
 *
 * The type `T` must be either a struct, enum or union.
 *
 * Example:
 *
 *  // MyModule.thrift
 *
 *  namespace cpp2 My.Namespace
 *
 *  struct MyStruct {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 *  enum MyEnum { a, b, c }
 *
 *  // C++
 *
 *  // yields `My::Namespace::MyModule_tags::metadata`
 *  using result1 = reflect_module_tag<MyStruct>;
 *
 *  // yields `My::Namespace::MyModule_tags::metadata`
 *  using result2 = reflect_module_tag<MyEnum>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
using reflect_module_tag = typename detail::reflect_module_tag_impl<
  T
>::template get<>::type;

/**
 * Tries to get the reflection metadata tag for the Thrift file where the type
 * `T` is declared.
 *
 * If the type `T` is a struct, enum or union and there is reflection
 * metadata available, the reflection metadata tag is returned. Otherwise,
 * returns `Default`.
 *
 * Example:
 *
 *  // MyModule.thrift
 *
 *  namespace cpp2 My.Namespace
 *
 *  struct MyStruct {
 *    1: i32 a
 *    2: string b
 *    3: double c
 *  }
 *
 *  enum MyEnum { a, b, c }
 *
 *  // C++
 *
 *  // yields `My::Namespace::MyModule_tags::metadata`
 *  using result1 = reflect_module_tag<MyStruct, void>;
 *
 *  // yields `My::Namespace::MyModule_tags::metadata`
 *  using result2 = reflect_module_tag<MyEnum, void>;
 *
 *  // yields `void`
 *  using result3 = reflect_module_tag<int, void>;
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T, typename Default>
using try_reflect_module_tag = typename detail::reflect_module_tag_impl<
  T
>::template try_get<Default>::type;

}} // apache::thrift

#include <thrift/lib/cpp2/fatal/reflection-inl-post.h>

#endif // THRIFT_FATAL_REFLECTION_H_
