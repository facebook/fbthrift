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

#include <thrift/lib/cpp2/type/ThriftType.h>

#include <folly/portability/GTest.h>
#include <thrift/conformance/if/gen-cpp2/object_fatal_all.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>

namespace apache::thrift::type {
using conformance::Object;
using conformance::Value;
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Bool);
  EXPECT_EQ(getName<tag>(), "bool");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::standard_type, bool>();
  IsSameType<tag::standard_types, detail::types<bool>>();
  EXPECT_EQ(toTType(BaseType::Bool), TType::T_BOOL);
  EXPECT_EQ(toThriftBaseType(TType::T_BOOL), BaseType::Bool);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Byte);
  EXPECT_EQ(getName<tag>(), "byte");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::standard_type, int8_t>();
  IsSameType<tag::standard_types, detail::types<int8_t, uint8_t>>();
  EXPECT_EQ(toTType(BaseType::Byte), TType::T_BYTE);
  EXPECT_EQ(toThriftBaseType(TType::T_BYTE), BaseType::Byte);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::I16);
  EXPECT_EQ(getName<tag>(), "i16");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::standard_type, int16_t>();
  IsSameType<tag::standard_types, detail::types<int16_t, uint16_t>>();
  EXPECT_EQ(toTType(BaseType::I16), TType::T_I16);
  EXPECT_EQ(toThriftBaseType(TType::T_I16), BaseType::I16);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::I32);
  EXPECT_EQ(getName<tag>(), "i32");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::standard_type, int32_t>();
  IsSameType<tag::standard_types, detail::types<int32_t, uint32_t>>();
  EXPECT_EQ(toTType(BaseType::I32), TType::T_I32);
  EXPECT_EQ(toThriftBaseType(TType::T_I32), BaseType::I32);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::I64);
  EXPECT_EQ(getName<tag>(), "i64");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::standard_type, int64_t>();
  IsSameType<tag::standard_types, detail::types<int64_t, uint64_t>>();
  EXPECT_EQ(toTType(BaseType::I64), TType::T_I64);
  EXPECT_EQ(toThriftBaseType(TType::T_I64), BaseType::I64);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_TRUE(integral_types::contains_bt<BaseType::Enum>);
  EXPECT_EQ(tag::kBaseType, BaseType::Enum);
  EXPECT_EQ(getName<tag>(), "enum");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_t = type::enum_t<BaseType>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Enum);
  EXPECT_EQ(getName<tag_t>(), "type.BaseType");
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::standard_type, BaseType>();
  IsSameType<tag_t::standard_types, detail::types<BaseType>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::Enum), TType::T_I32);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Float);
  EXPECT_EQ(getName<tag>(), "float");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::standard_type, float>();
  IsSameType<tag::standard_types, detail::types<float>>();
  EXPECT_EQ(toTType(BaseType::Float), TType::T_FLOAT);
  EXPECT_EQ(toThriftBaseType(TType::T_FLOAT), BaseType::Float);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Double);
  EXPECT_EQ(getName<tag>(), "double");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::standard_type, double>();
  IsSameType<tag::standard_types, detail::types<double>>();
  EXPECT_EQ(toTType(BaseType::Double), TType::T_DOUBLE);
  EXPECT_EQ(toThriftBaseType(TType::T_DOUBLE), BaseType::Double);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::String);
  EXPECT_EQ(getName<tag>(), "string");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::standard_type, std::string>();
  IsSameType<
      tag::standard_types,
      detail::types<std::string, std::string_view, folly::StringPiece>>();
  EXPECT_EQ(toTType(BaseType::String), TType::T_UTF8);
  EXPECT_EQ(toThriftBaseType(TType::T_UTF8), BaseType::String);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Binary);
  EXPECT_EQ(getName<tag>(), "binary");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<tag::standard_type, std::string>();
  IsSameType<
      tag::standard_types,
      detail::types<std::string, folly::IOBuf, folly::ByteRange>>();
  EXPECT_EQ(toTType(BaseType::Binary), TType::T_STRING);
  EXPECT_EQ(toThriftBaseType(TType::T_STRING), BaseType::Binary);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Struct);
  EXPECT_EQ(getName<tag>(), "struct");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_t = type::struct_t<Object>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Struct);
  EXPECT_EQ(getName<tag_t>(), "object.Object");
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::standard_type, Object>();
  IsSameType<tag_t::standard_types, detail::types<Object>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::Struct), TType::T_STRUCT);
  EXPECT_EQ(toThriftBaseType(TType::T_STRUCT), BaseType::Struct);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Union);
  EXPECT_EQ(getName<tag>(), "union");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_t = type::union_t<Value>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Union);
  EXPECT_EQ(getName<tag_t>(), "object.Value");
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::standard_type, Value>();
  IsSameType<tag_t::standard_types, detail::types<Value>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::Union), TType::T_STRUCT);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Exception);
  EXPECT_EQ(getName<tag>(), "exception");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  EXPECT_EQ(toTType(BaseType::Exception), TType::T_STRUCT);

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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::List);
  EXPECT_EQ(getName<tag>(), "list");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_c = type::list<type::struct_c>;
  EXPECT_EQ(tag_c::kBaseType, BaseType::List);
  EXPECT_EQ(getName<tag_c>(), "list<struct>");
  EXPECT_FALSE(is_concrete_type_v<tag_c>);
  EXPECT_TRUE(all_types::contains<tag_c>);

  using tag_t = type::list<type::string_t>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::List);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  EXPECT_EQ(getName<tag_t>(), "list<string>");
  IsSameType<tag_t::standard_type, std::vector<std::string>>();
  IsSameType<
      tag_t::standard_types,
      detail::types<
          std::vector<std::string>,
          std::vector<std::string_view>,
          std::vector<folly::StringPiece>>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::List), TType::T_LIST);
  EXPECT_EQ(toThriftBaseType(TType::T_LIST), BaseType::List);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Set);
  EXPECT_EQ(getName<tag>(), "set");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_c = type::set<type::struct_c>;
  EXPECT_EQ(tag_c::kBaseType, BaseType::Set);
  EXPECT_EQ(getName<tag_c>(), "set<struct>");
  EXPECT_FALSE(is_concrete_type_v<tag_c>);
  EXPECT_TRUE(all_types::contains<tag_c>);

  using tag_t = type::set<type::string_t>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Set);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  EXPECT_EQ(getName<tag_t>(), "set<string>");
  IsSameType<tag_t::standard_type, std::set<std::string>>();
  IsSameType<
      tag_t::standard_types,
      detail::types<
          std::set<std::string>,
          std::set<std::string_view>,
          std::set<folly::StringPiece>>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::Set), TType::T_SET);
  EXPECT_EQ(toThriftBaseType(TType::T_SET), BaseType::Set);
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
  EXPECT_TRUE(all_types::contains<tag>);

  EXPECT_EQ(tag::kBaseType, BaseType::Map);
  EXPECT_EQ(getName<tag>(), "map");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_c = type::map<type::struct_c, type::struct_c>;
  EXPECT_EQ(tag_c::kBaseType, BaseType::Map);
  EXPECT_EQ(getName<tag_c>(), "map<struct, struct>");
  EXPECT_FALSE(is_concrete_type_v<tag_c>);
  EXPECT_TRUE(all_types::contains<tag_c>);

  using tag_kc = type::map<type::struct_c, type::byte_t>;
  EXPECT_EQ(tag_kc::kBaseType, BaseType::Map);
  EXPECT_EQ(getName<tag_kc>(), "map<struct, byte>");
  EXPECT_FALSE(is_concrete_type_v<tag_kc>);
  EXPECT_TRUE(all_types::contains<tag_kc>);

  using tag_vc = type::map<type::byte_t, type::struct_c>;
  EXPECT_EQ(tag_vc::kBaseType, BaseType::Map);
  EXPECT_EQ(getName<tag_vc>(), "map<byte, struct>");
  EXPECT_FALSE(is_concrete_type_v<tag_vc>);
  EXPECT_TRUE(all_types::contains<tag_vc>);

  using tag_t = type::map<type::i16_t, type::i32_t>;
  EXPECT_EQ(tag_t::kBaseType, BaseType::Map);
  EXPECT_EQ(getName<tag_t>(), "map<i16, i32>");
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<tag_t::standard_type, std::map<int16_t, int32_t>>();
  IsSameType<
      tag_t::standard_types,
      detail::types<std::map<int16_t, int32_t>, std::map<int16_t, uint32_t>>>();
  EXPECT_TRUE(all_types::contains<tag_t>);

  EXPECT_EQ(toTType(BaseType::Map), TType::T_MAP);
  EXPECT_EQ(toThriftBaseType(TType::T_MAP), BaseType::Map);
}

TEST(ThriftTypesTest, ConcreteType_Bound) {
  IsSameType<
      fatal::filter<all_types, bound::is_concrete_type>,
      detail::types<
          type::bool_t,
          type::byte_t,
          type::i16_t,
          type::i32_t,
          type::i64_t,
          type::float_t,
          type::double_t,
          type::string_t,
          type::binary_t>>();
}

TEST(ThriftTypesTest, IsStandardType) {
  EXPECT_TRUE((is_standard_type<type::i32_t, int32_t>::value));
  EXPECT_TRUE((is_standard_type<type::i32_t, uint32_t>::value));
  EXPECT_FALSE((is_standard_type<type::i32_t, int16_t>::value));
}

} // namespace
} // namespace apache::thrift::type
