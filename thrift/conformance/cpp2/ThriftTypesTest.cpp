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
#include <thrift/conformance/if/gen-cpp2/object_types.h>

namespace apache::thrift::conformance {
namespace {

template <typename actual, typename expected>
struct IsSameType;

template <typename T>
struct IsSameType<T, T> {};

TEST(ThriftTypesTest, Bool) {
  using tag = type::bool_t;
  EXPECT_TRUE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_TRUE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Bool);
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::native_type, bool>();
  IsSameType<tag::native_types, types<bool>>();
}

TEST(ThriftTypesTest, Byte) {
  using tag = type::byte_t;
  EXPECT_TRUE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_TRUE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Byte);
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::native_type, int8_t>();
  IsSameType<tag::native_types, types<int8_t, uint8_t>>();
}

TEST(ThriftTypesTest, I16) {
  using tag = type::i16_t;
  EXPECT_TRUE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_TRUE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::I16);
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::native_type, int16_t>();
  IsSameType<tag::native_types, types<int16_t, uint16_t>>();
}

TEST(ThriftTypesTest, I32) {
  using tag = type::i32_t;
  EXPECT_TRUE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_TRUE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::I32);
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::native_type, int32_t>();
  IsSameType<tag::native_types, types<int32_t, uint32_t>>();
}

TEST(ThriftTypesTest, I64) {
  using tag = type::i64_t;
  EXPECT_TRUE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_TRUE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::I64);
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::native_type, int64_t>();
  IsSameType<tag::native_types, types<int64_t, uint64_t>>();
}

TEST(ThriftTypesTest, Enum) {
  using tag = type::enum_c;
  EXPECT_TRUE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_TRUE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);

  EXPECT_TRUE(integral_types::contains_bt<BaseType::Enum>);
  EXPECT_EQ(tag::kBaseType, BaseType::Enum);
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_t = type::enum_t<BaseType>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Enum);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::native_type, BaseType>();
  IsSameType<tag_t::native_types, types<BaseType>>();
}

TEST(ThriftTypesTest, Float) {
  using tag = type::float_t;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_TRUE(floating_point_types::contains<tag>);
  EXPECT_TRUE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);
}

TEST(ThriftTypesTest, Double) {
  using tag = type::double_t;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_TRUE(floating_point_types::contains<tag>);
  EXPECT_TRUE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);
}

TEST(ThriftTypesTest, String) {
  using tag = type::string_t;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_FALSE(numeric_types::contains<tag>);
  EXPECT_TRUE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);
}

TEST(ThriftTypesTest, Binary) {
  using tag = type::binary_t;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_FALSE(numeric_types::contains<tag>);
  EXPECT_TRUE(string_types::contains<tag>);
  EXPECT_TRUE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_FALSE(composite_types::contains<tag>);
}

TEST(ThriftTypesTest, Struct) {
  using tag = type::struct_c;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_FALSE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_FALSE(primitive_types::contains<tag>);
  EXPECT_TRUE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_TRUE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Struct);
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_t = type::struct_t<Object>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Struct);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::native_type, Object>();
  IsSameType<tag_t::native_types, types<Object>>();
}

TEST(ThriftTypesTest, Union) {
  using tag = type::union_c;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_FALSE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_FALSE(primitive_types::contains<tag>);
  EXPECT_TRUE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_TRUE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Union);
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_t = type::union_t<Value>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Union);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::native_type, Value>();
  IsSameType<tag_t::native_types, types<Value>>();
}

TEST(ThriftTypesTest, Exeption) {
  using tag = type::exception_c;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_FALSE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_FALSE(primitive_types::contains<tag>);
  EXPECT_TRUE(structured_types::contains<tag>);
  EXPECT_TRUE(singular_types::contains<tag>);
  EXPECT_FALSE(container_types::contains<tag>);
  EXPECT_TRUE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Exception);
  EXPECT_FALSE(is_concrete_type_v<tag>);

  // TODO(afuller): Add a test exception and test the concrete form.
}

TEST(ThriftTypesTest, List) {
  using tag = type::list_c;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_FALSE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_FALSE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_FALSE(singular_types::contains<tag>);
  EXPECT_TRUE(container_types::contains<tag>);
  EXPECT_TRUE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::List);
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_c = type::list<type::struct_c>;
  EXPECT_EQ(tag_c::kBaseType, BaseType::List);
  EXPECT_FALSE(is_concrete_type_v<tag_c>);

  using tag_t = type::list<type::string_t>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::List);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::native_type, std::vector<std::string>>();
  IsSameType<
      tag_t::native_types,
      types<
          std::vector<std::string>,
          std::vector<std::string_view>,
          std::vector<folly::StringPiece>>>();
}

TEST(ThriftTypesTest, Set) {
  using tag = type::set_c;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_FALSE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_FALSE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_FALSE(singular_types::contains<tag>);
  EXPECT_TRUE(container_types::contains<tag>);
  EXPECT_TRUE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Set);
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_c = type::set<type::struct_c>;
  EXPECT_EQ(tag_c::kBaseType, BaseType::Set);
  EXPECT_FALSE(is_concrete_type_v<tag_c>);

  using tag_t = type::set<type::string_t>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Set);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::native_type, std::set<std::string>>();
  IsSameType<
      tag_t::native_types,
      types<
          std::set<std::string>,
          std::set<std::string_view>,
          std::set<folly::StringPiece>>>();
}

TEST(ThriftTypesTest, Map) {
  using tag = type::map_c;
  EXPECT_FALSE(integral_types::contains<tag>);
  EXPECT_FALSE(floating_point_types::contains<tag>);
  EXPECT_FALSE(numeric_types::contains<tag>);
  EXPECT_FALSE(string_types::contains<tag>);
  EXPECT_FALSE(primitive_types::contains<tag>);
  EXPECT_FALSE(structured_types::contains<tag>);
  EXPECT_FALSE(singular_types::contains<tag>);
  EXPECT_TRUE(container_types::contains<tag>);
  EXPECT_TRUE(composite_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Map);
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_c = type::map<type::struct_c, type::struct_c>;
  EXPECT_EQ(tag_c::kBaseType, BaseType::Map);
  EXPECT_FALSE(is_concrete_type_v<tag_c>);

  using tag_kc = type::map<type::struct_c, type::byte_t>;
  EXPECT_EQ(tag_kc::kBaseType, BaseType::Map);
  EXPECT_FALSE(is_concrete_type_v<tag_kc>);

  using tag_vc = type::map<type::byte_t, type::struct_c>;
  EXPECT_EQ(tag_vc::kBaseType, BaseType::Map);
  EXPECT_FALSE(is_concrete_type_v<tag_vc>);

  using tag_t = type::map<type::i16_t, type::i32_t>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Map);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::native_type, std::map<int16_t, int32_t>>();
  IsSameType<
      tag_t::native_types,
      types<std::map<int16_t, int32_t>, std::map<int16_t, uint32_t>>>();
}

} // namespace
} // namespace apache::thrift::conformance
