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

#include <thrift/conformance/cpp2/ThriftTypes.h>

#include <gtest/gtest.h>

namespace apache::thrift::conformance {
namespace {

TEST(ThriftTypesTest, Bool) {
  using tag_t = type::bool_t;
  EXPECT_TRUE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_TRUE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Byte) {
  using tag_t = type::byte_t;
  EXPECT_TRUE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_TRUE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, I16) {
  using tag_t = type::i16_t;
  EXPECT_TRUE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_TRUE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, I32) {
  using tag_t = type::i32_t;
  EXPECT_TRUE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_TRUE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, I64) {
  using tag_t = type::i64_t;
  EXPECT_TRUE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_TRUE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Enum) {
  using tag_t = type::enum_t;
  EXPECT_TRUE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_TRUE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Float) {
  using tag_t = type::float_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_TRUE(floating_point_types::contains<tag_t>);
  EXPECT_TRUE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Double) {
  using tag_t = type::double_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_TRUE(floating_point_types::contains<tag_t>);
  EXPECT_TRUE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, String) {
  using tag_t = type::string_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_FALSE(numeric_types::contains<tag_t>);
  EXPECT_TRUE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Binary) {
  using tag_t = type::binary_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_FALSE(numeric_types::contains<tag_t>);
  EXPECT_TRUE(string_types::contains<tag_t>);
  EXPECT_TRUE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_FALSE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Struct) {
  using tag_t = type::struct_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_FALSE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_FALSE(primitive_types::contains<tag_t>);
  EXPECT_TRUE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_TRUE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Union) {
  using tag_t = type::union_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_FALSE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_FALSE(primitive_types::contains<tag_t>);
  EXPECT_TRUE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_TRUE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Exeption) {
  using tag_t = type::exception_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_FALSE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_FALSE(primitive_types::contains<tag_t>);
  EXPECT_TRUE(structured_types::contains<tag_t>);
  EXPECT_TRUE(singular_types::contains<tag_t>);
  EXPECT_FALSE(container_types::contains<tag_t>);
  EXPECT_TRUE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, List) {
  using tag_t = type::list_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_FALSE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_FALSE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_FALSE(singular_types::contains<tag_t>);
  EXPECT_TRUE(container_types::contains<tag_t>);
  EXPECT_TRUE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Set) {
  using tag_t = type::set_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_FALSE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_FALSE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_FALSE(singular_types::contains<tag_t>);
  EXPECT_TRUE(container_types::contains<tag_t>);
  EXPECT_TRUE(composite_types::contains<tag_t>);
}

TEST(ThriftTypesTest, Map) {
  using tag_t = type::map_t;
  EXPECT_FALSE(integral_types::contains<tag_t>);
  EXPECT_FALSE(floating_point_types::contains<tag_t>);
  EXPECT_FALSE(numeric_types::contains<tag_t>);
  EXPECT_FALSE(string_types::contains<tag_t>);
  EXPECT_FALSE(primitive_types::contains<tag_t>);
  EXPECT_FALSE(structured_types::contains<tag_t>);
  EXPECT_FALSE(singular_types::contains<tag_t>);
  EXPECT_TRUE(container_types::contains<tag_t>);
  EXPECT_TRUE(composite_types::contains<tag_t>);
}

} // namespace
} // namespace apache::thrift::conformance
