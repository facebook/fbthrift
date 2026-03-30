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

#include <cstdint>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/test/gen-cpp2/Service_types.h>
#include <thrift/lib/cpp2/test/util/gen-cpp2/enum_types.h>
#include <thrift/lib/cpp2/type/detail/TypeClassToTypeTag.h>

namespace apache::thrift::type_class {

using Enum = test::MyEnum;
using Struct = test::TestStruct;
using Union = test::TestUnsignedIntUnion;

TEST(ToTypeTag, IntegralStandard) {
  static_assert(std::is_same_v<to_type_tag_t<integral, bool>, type::bool_t>);
  static_assert(
      std::is_same_v<to_type_tag_t<integral, std::int8_t>, type::byte_t>);
  static_assert(
      std::is_same_v<to_type_tag_t<integral, std::int16_t>, type::i16_t>);
  static_assert(
      std::is_same_v<to_type_tag_t<integral, std::int32_t>, type::i32_t>);
  static_assert(
      std::is_same_v<to_type_tag_t<integral, std::int64_t>, type::i64_t>);
}

TEST(ToTypeTag, IntegralCppType) {
  static_assert(std::is_same_v<
                to_type_tag_t<integral, std::uint8_t>,
                type::cpp_type<std::uint8_t, type::byte_t>>);
  static_assert(std::is_same_v<
                to_type_tag_t<integral, std::uint16_t>,
                type::cpp_type<std::uint16_t, type::i16_t>>);
  static_assert(std::is_same_v<
                to_type_tag_t<integral, std::uint32_t>,
                type::cpp_type<std::uint32_t, type::i32_t>>);
  static_assert(std::is_same_v<
                to_type_tag_t<integral, std::uint64_t>,
                type::cpp_type<std::uint64_t, type::i64_t>>);
}

TEST(ToTypeTag, FloatingPointStandard) {
  static_assert(
      std::is_same_v<to_type_tag_t<floating_point, float>, type::float_t>);
  static_assert(
      std::is_same_v<to_type_tag_t<floating_point, double>, type::double_t>);
}

TEST(ToTypeTag, FloatingPointCppType) {
  static_assert(std::is_same_v<
                to_type_tag_t<floating_point, std::int32_t>,
                type::cpp_type<std::int32_t, type::float_t>>);
  static_assert(std::is_same_v<
                to_type_tag_t<floating_point, std::int64_t>,
                type::cpp_type<std::int64_t, type::double_t>>);
}

TEST(ToTypeTag, StringStandard) {
  static_assert(
      std::is_same_v<to_type_tag_t<string, std::string>, type::string_t>);
  static_assert(
      std::is_same_v<to_type_tag_t<binary, std::string>, type::binary_t>);
}

TEST(ToTypeTag, StringCppType) {
  struct MyString {};
  static_assert(std::is_same_v<
                to_type_tag_t<string, MyString>,
                type::cpp_type<MyString, type::string_t>>);
  static_assert(std::is_same_v<
                to_type_tag_t<binary, MyString>,
                type::cpp_type<MyString, type::binary_t>>);
}

TEST(ToTypeTag, NamedTypesStandard) {
  static_assert(
      std::is_same_v<to_type_tag_t<enumeration, Enum>, type::enum_t<Enum>>);
  static_assert(
      std::is_same_v<to_type_tag_t<structure, Struct>, type::struct_t<Struct>>);
  static_assert(
      std::is_same_v<to_type_tag_t<variant, Union>, type::union_t<Union>>);
}

TEST(ToTypeTag, NamedTypesCppType) {
  enum class MyEnum {};
  class MyStruct {};
  class MyUnion {};
  static_assert(std::is_same_v<
                to_type_tag_t<enumeration, MyEnum>,
                type::cpp_type<MyEnum, type::enum_t<MyEnum>>>);
  static_assert(std::is_same_v<
                to_type_tag_t<structure, MyStruct>,
                type::cpp_type<MyStruct, type::struct_t<MyStruct>>>);
  static_assert(std::is_same_v<
                to_type_tag_t<variant, MyUnion>,
                type::cpp_type<MyUnion, type::union_t<MyUnion>>>);
}

TEST(ToTypeTag, ContainersStandard) {
  static_assert(
      std::is_same_v<
          to_type_tag_t<list<set<structure>>, std::vector<std::set<Struct>>>,
          type::list<type::set<type::struct_t<Struct>>>>);
  static_assert(std::is_same_v<
                to_type_tag_t<
                    map<string, list<integral>>,
                    std::map<std::string, std::vector<std::int32_t>>>,
                type::map<type::string_t, type::list<type::i32_t>>>);

  // Standard outer container with non-standard inner element type
  static_assert(std::is_same_v<
                to_type_tag_t<list<integral>, std::vector<std::uint32_t>>,
                type::list<type::cpp_type<std::uint32_t, type::i32_t>>>);
  static_assert(std::is_same_v<
                to_type_tag_t<
                    map<integral, set<integral>>,
                    std::map<std::uint32_t, std::set<std::uint16_t>>>,
                type::map<
                    type::cpp_type<std::uint32_t, type::i32_t>,
                    type::set<type::cpp_type<std::uint16_t, type::i16_t>>>>);
}

TEST(ToTypeTag, ContainersCppType) {
  using ListTag = type::list<type::struct_t<Struct>>;
  static_assert(std::is_same_v<
                to_type_tag_t<list<structure>, std::deque<Struct>>,
                type::cpp_type<std::deque<Struct>, ListTag>>);

  using SetTag = type::set<type::i32_t>;
  static_assert(std::is_same_v<
                to_type_tag_t<set<integral>, std::unordered_set<std::int32_t>>,
                type::cpp_type<std::unordered_set<std::int32_t>, SetTag>>);

  using MapTag = type::map<type::string_t, type::i32_t>;
  static_assert(
      std::is_same_v<
          to_type_tag_t<
              map<string, integral>,
              std::unordered_map<std::string, std::int32_t>>,
          type::
              cpp_type<std::unordered_map<std::string, std::int32_t>, MapTag>>);
}

} // namespace apache::thrift::type_class
