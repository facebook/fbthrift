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

#include <thrift/lib/cpp2/fatal/reflect_category.h>

#include <thrift/test/gen-cpp2/reflection_fatal.h>
#include <thrift/test/gen-cpp2/reflection_fatal_enum.h>
#include <thrift/test/gen-cpp2/reflection_fatal_struct.h>
#include <thrift/test/gen-cpp2/reflection_fatal_union.h>

#include <thrift/test/expect_same.h>

#include <gtest/gtest.h>

namespace test_cpp2 {
namespace cpp_reflection {

template <apache::thrift::thrift_category Category>
using category = std::integral_constant<
  apache::thrift::thrift_category,
  Category
>;

TEST(reflection, is_reflectable_module) {
  EXPECT_SAME<
    std::true_type,
    apache::thrift::is_reflectable_module<reflection_tags::metadata>
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
    apache::thrift::is_reflectable_struct<reflection_tags::metadata>
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
    apache::thrift::is_reflectable_enum<reflection_tags::metadata>
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
    apache::thrift::is_reflectable_union<reflection_tags::metadata>
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

TEST(reflection, reflect_category) {
  EXPECT_SAME<
    category<apache::thrift::thrift_category::unknown>,
    apache::thrift::reflect_category<reflection_tags::metadata>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::nothing>,
    apache::thrift::reflect_category<void>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<signed char>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<signed short>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<signed int>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<signed long>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<signed long long>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<bool>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<unsigned char>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<unsigned short>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<unsigned int>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<unsigned long>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<unsigned long long>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::int8_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::int16_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::int32_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::int64_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::intmax_t>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::uint8_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::uint16_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::uint32_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::uint64_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::uintmax_t>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::size_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::intptr_t>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::integral>,
    apache::thrift::reflect_category<std::uintptr_t>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::floating_point>,
    apache::thrift::reflect_category<float>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::floating_point>,
    apache::thrift::reflect_category<double>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::floating_point>,
    apache::thrift::reflect_category<long double>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::string>,
    apache::thrift::reflect_category<std::string>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::enumeration>,
    apache::thrift::reflect_category<enum1>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::enumeration>,
    apache::thrift::reflect_category<enum2>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::enumeration>,
    apache::thrift::reflect_category<enum3>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::variant>,
    apache::thrift::reflect_category<union1>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::variant>,
    apache::thrift::reflect_category<union2>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::variant>,
    apache::thrift::reflect_category<union3>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::structure>,
    apache::thrift::reflect_category<struct1>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::structure>,
    apache::thrift::reflect_category<struct2>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::structure>,
    apache::thrift::reflect_category<struct3>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::list>,
    apache::thrift::reflect_category<std::vector<int>>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::list>,
    apache::thrift::reflect_category<std::vector<std::string>>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::list>,
    apache::thrift::reflect_category<std::vector<struct1>>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::set>,
    apache::thrift::reflect_category<std::set<int>>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::set>,
    apache::thrift::reflect_category<std::set<std::string>>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::set>,
    apache::thrift::reflect_category<std::set<struct1>>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::set>,
    apache::thrift::reflect_category<std::unordered_set<int>>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::set>,
    apache::thrift::reflect_category<std::unordered_set<std::string>>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::map>,
    apache::thrift::reflect_category<std::map<int, std::string>>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::map>,
    apache::thrift::reflect_category<std::map<std::string, struct1>>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::map>,
    apache::thrift::reflect_category<std::map<struct1, struct2>>
  >();

  EXPECT_SAME<
    category<apache::thrift::thrift_category::map>,
    apache::thrift::reflect_category<std::unordered_map<int, std::string>>
  >();
  EXPECT_SAME<
    category<apache::thrift::thrift_category::map>,
    apache::thrift::reflect_category<std::unordered_map<std::string, struct1>>
  >();
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
