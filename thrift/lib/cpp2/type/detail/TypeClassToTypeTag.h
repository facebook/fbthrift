/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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
#include <type_traits>
#include <vector>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Tag.h>

namespace apache::thrift::type_class {

namespace detail {

template <
    class CppType,
    class Tag,
    class StandardType = type::standard_type<Tag>>
using maybe_cpp_type = std::conditional_t<
    std::is_same_v<CppType, StandardType>,
    Tag,
    type::cpp_type<CppType, Tag>>;
} // namespace detail

template <class TypeClass, class CppType>
struct to_type_tag;

template <class... T>
using to_type_tag_t = typename to_type_tag<T...>::type;

template <>
struct to_type_tag<integral, bool> {
  using type = type::bool_t;
};

template <class CppType>
struct to_type_tag<integral, CppType> {
  static_assert(
      sizeof(CppType) == 1 || sizeof(CppType) == 2 || sizeof(CppType) == 4 ||
      sizeof(CppType) == 8);
  using type = detail::maybe_cpp_type<
      CppType,
      std::conditional_t<
          sizeof(CppType) == 1,
          type::byte_t,
          std::conditional_t<
              sizeof(CppType) == 2,
              type::i16_t,
              std::conditional_t<
                  sizeof(CppType) == 4,
                  type::i32_t,
                  type::i64_t>>>>;
};

template <class CppType>
struct to_type_tag<floating_point, CppType> {
  static_assert(
      sizeof(CppType) == sizeof(float) || sizeof(CppType) == sizeof(double));
  using type = detail::maybe_cpp_type<
      CppType,
      std::conditional_t<
          sizeof(CppType) == sizeof(float),
          type::float_t,
          type::double_t>>;
};

template <class CppType>
struct to_type_tag<string, CppType> {
  using type = detail::maybe_cpp_type<CppType, type::string_t>;
};

template <class CppType>
struct to_type_tag<binary, CppType> {
  using type = detail::maybe_cpp_type<CppType, type::binary_t>;
};

template <class CppType>
struct to_type_tag<enumeration, CppType> {
  using type = std::conditional_t<
      is_thrift_enum_v<CppType>,
      type::enum_t<CppType>,
      type::cpp_type<CppType, type::enum_t<CppType>>>;
};

template <class CppType>
struct to_type_tag<structure, CppType> {
  using type = std::conditional_t<
      is_thrift_struct_v<CppType>,
      type::struct_t<CppType>,
      type::cpp_type<CppType, type::struct_t<CppType>>>;
};

template <class CppType>
struct to_type_tag<variant, CppType> {
  using type = std::conditional_t<
      is_thrift_union_v<CppType>,
      type::union_t<CppType>,
      type::cpp_type<CppType, type::union_t<CppType>>>;
};

template <class ValueTC, class CppType>
struct to_type_tag<list<ValueTC>, CppType> {
  using type = detail::maybe_cpp_type<
      CppType,
      type::list<to_type_tag_t<ValueTC, typename CppType::value_type>>,
      std::vector<typename CppType::value_type>>;
};

template <class ValueTC, class CppType>
struct to_type_tag<set<ValueTC>, CppType> {
  using type = detail::maybe_cpp_type<
      CppType,
      type::set<to_type_tag_t<ValueTC, typename CppType::value_type>>,
      std::set<typename CppType::value_type>>;
};

template <class KeyTC, class ValueTC, class CppType>
struct to_type_tag<map<KeyTC, ValueTC>, CppType> {
  using type = detail::maybe_cpp_type<
      CppType,
      type::map<
          to_type_tag_t<KeyTC, typename CppType::key_type>,
          to_type_tag_t<ValueTC, typename CppType::mapped_type>>,
      std::map<typename CppType::key_type, typename CppType::mapped_type>>;
};

} // namespace apache::thrift::type_class
