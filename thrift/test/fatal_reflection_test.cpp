/*
 * Copyright 2016 Facebook, Inc.
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

#include <thrift/lib/cpp2/fatal/container_traits.h>

#include <thrift/test/gen-cpp2/reflection_fatal.h>
#include <thrift/test/gen-cpp2/reflection_fatal_types.h>

#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

#include <gtest/gtest.h>

namespace test_cpp2 {
namespace cpp_reflection {

TEST(reflection, is_reflectable_module) {
  EXPECT_SAME<
    std::true_type,
    apache::thrift::is_reflectable_module<reflection_tags::module>
  >();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_module<enum1>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_module<enum2>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_module<enum3>>();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_module<union1>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_module<union2>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_module<union3>>();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<struct1>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<struct2>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<struct3>
  >();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_module<void>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_module<int>>();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::string>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::vector<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::vector<std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::vector<struct1>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::set<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::set<std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::set<struct1>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::unordered_set<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::unordered_set<std::string>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::map<int, std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::map<std::string, struct1>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::map<struct1, struct2>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<std::unordered_map<int, std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_module<
      std::unordered_map<std::string, struct1>
    >
  >();
}

TEST(reflection, is_reflectable_struct) {
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<reflection_tags::module>
  >();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_struct<enum1>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_struct<enum2>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_struct<enum3>>();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_struct<union1>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_struct<union2>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_struct<union3>>();

  EXPECT_SAME<std::true_type, apache::thrift::is_reflectable_struct<struct1>>();
  EXPECT_SAME<std::true_type, apache::thrift::is_reflectable_struct<struct2>>();
  EXPECT_SAME<std::true_type, apache::thrift::is_reflectable_struct<struct3>>();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_struct<void>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_struct<int>>();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::string>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::vector<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::vector<std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::vector<struct1>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::set<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::set<std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::set<struct1>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::unordered_set<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::unordered_set<std::string>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::map<int, std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::map<std::string, struct1>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::map<struct1, struct2>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<std::unordered_map<int, std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_struct<
      std::unordered_map<std::string, struct1>
    >
  >();
}

TEST(reflection, is_reflectable_enum) {
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<reflection_tags::module>
  >();

  EXPECT_SAME<std::true_type, apache::thrift::is_reflectable_enum<enum1>>();
  EXPECT_SAME<std::true_type, apache::thrift::is_reflectable_enum<enum2>>();
  EXPECT_SAME<std::true_type, apache::thrift::is_reflectable_enum<enum3>>();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_enum<union1>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_enum<union2>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_enum<union3>>();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_enum<struct1>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_enum<struct2>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_enum<struct3>>();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_enum<void>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_enum<int>>();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::string>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::vector<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::vector<std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::vector<struct1>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::set<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::set<std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::set<struct1>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::unordered_set<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::unordered_set<std::string>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::map<int, std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::map<std::string, struct1>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::map<struct1, struct2>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<std::unordered_map<int, std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_enum<
      std::unordered_map<std::string, struct1>
    >
  >();
}

TEST(reflection, is_reflectable_union) {
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<reflection_tags::module>
  >();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_union<enum1>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_union<enum2>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_union<enum3>>();

  EXPECT_SAME<std::true_type, apache::thrift::is_reflectable_union<union1>>();
  EXPECT_SAME<std::true_type, apache::thrift::is_reflectable_union<union2>>();
  EXPECT_SAME<std::true_type, apache::thrift::is_reflectable_union<union3>>();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_union<struct1>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_union<struct2>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_union<struct3>>();

  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_union<void>>();
  EXPECT_SAME<std::false_type, apache::thrift::is_reflectable_union<int>>();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::string>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::vector<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::vector<std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::vector<struct1>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::set<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::set<std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::set<struct1>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::unordered_set<int>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::unordered_set<std::string>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::map<int, std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::map<std::string, struct1>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::map<struct1, struct2>>
  >();

  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<std::unordered_map<int, std::string>>
  >();
  EXPECT_SAME<
    std::false_type,
    apache::thrift::is_reflectable_union<
      std::unordered_map<std::string, struct1>
    >
  >();
}

TEST(reflection, reflect_type_class) {
  EXPECT_SAME<
    apache::thrift::type_class::unknown,
    apache::thrift::reflect_type_class<reflection_tags::module>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::nothing,
    apache::thrift::reflect_type_class<void>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<signed char>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<signed short>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<signed int>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<signed long>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<signed long long>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<bool>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<unsigned char>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<unsigned short>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<unsigned int>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<unsigned long>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<unsigned long long>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::int8_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::int16_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::int32_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::int64_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::intmax_t>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::uint8_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::uint16_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::uint32_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::uint64_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::uintmax_t>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::size_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::intptr_t>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::integral,
    apache::thrift::reflect_type_class<std::uintptr_t>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::floating_point,
    apache::thrift::reflect_type_class<float>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::floating_point,
    apache::thrift::reflect_type_class<double>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::floating_point,
    apache::thrift::reflect_type_class<long double>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::string,
    apache::thrift::reflect_type_class<std::string>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::enumeration,
    apache::thrift::reflect_type_class<enum1>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::enumeration,
    apache::thrift::reflect_type_class<enum2>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::enumeration,
    apache::thrift::reflect_type_class<enum3>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::variant,
    apache::thrift::reflect_type_class<union1>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::variant,
    apache::thrift::reflect_type_class<union2>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::variant,
    apache::thrift::reflect_type_class<union3>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::structure,
    apache::thrift::reflect_type_class<struct1>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::structure,
    apache::thrift::reflect_type_class<struct2>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::structure,
    apache::thrift::reflect_type_class<struct3>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::list<apache::thrift::type_class::integral>,
    apache::thrift::reflect_type_class<std::vector<int>>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::list<apache::thrift::type_class::string>,
    apache::thrift::reflect_type_class<std::vector<std::string>>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::list<apache::thrift::type_class::structure>,
    apache::thrift::reflect_type_class<std::vector<struct1>>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::set<apache::thrift::type_class::integral>,
    apache::thrift::reflect_type_class<std::set<int>>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::set<apache::thrift::type_class::string>,
    apache::thrift::reflect_type_class<std::set<std::string>>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::set<apache::thrift::type_class::structure>,
    apache::thrift::reflect_type_class<std::set<struct1>>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::set<apache::thrift::type_class::integral>,
    apache::thrift::reflect_type_class<std::unordered_set<int>>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::set<apache::thrift::type_class::string>,
    apache::thrift::reflect_type_class<std::unordered_set<std::string>>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::map<
      apache::thrift::type_class::integral,
      apache::thrift::type_class::string
    >,
    apache::thrift::reflect_type_class<std::map<int, std::string>>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::map<
      apache::thrift::type_class::string,
      apache::thrift::type_class::structure
    >,
    apache::thrift::reflect_type_class<std::map<std::string, struct1>>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::map<
      apache::thrift::type_class::structure,
      apache::thrift::type_class::structure
    >,
    apache::thrift::reflect_type_class<std::map<struct1, struct2>>
  >();

  EXPECT_SAME<
    apache::thrift::type_class::map<
      apache::thrift::type_class::integral,
      apache::thrift::type_class::string
    >,
    apache::thrift::reflect_type_class<std::unordered_map<int, std::string>>
  >();
  EXPECT_SAME<
    apache::thrift::type_class::map<
      apache::thrift::type_class::string,
      apache::thrift::type_class::structure
    >,
    apache::thrift::reflect_type_class<std::unordered_map<std::string, struct1>>
  >();
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
