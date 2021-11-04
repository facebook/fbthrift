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

#include <thrift/lib/cpp2/type/Traits.h>

#include <folly/portability/GTest.h>
#include <thrift/conformance/if/gen-cpp2/object_fatal_all.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>
#include <thrift/lib/cpp2/type/ThriftType.h>

namespace apache::thrift::type {
namespace {
using conformance::Object;
using conformance::Value;

template <typename actual, typename expected>
struct IsSameType;

template <typename T>
struct IsSameType<T, T> {};

TEST(TraitsTest, Bool) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Bool);
  EXPECT_EQ(getName<tag>(), "bool");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<standard_type<tag>, bool>();
  IsSameType<standard_types<tag>, detail::types<bool>>();
  EXPECT_EQ(toTType(BaseType::Bool), TType::T_BOOL);
  EXPECT_EQ(toThriftBaseType(TType::T_BOOL), BaseType::Bool);
}

TEST(TraitsTest, Byte) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Byte);
  EXPECT_EQ(getName<tag>(), "byte");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<standard_type<tag>, int8_t>();
  IsSameType<standard_types<tag>, detail::types<int8_t, uint8_t>>();
  EXPECT_EQ(toTType(BaseType::Byte), TType::T_BYTE);
  EXPECT_EQ(toThriftBaseType(TType::T_BYTE), BaseType::Byte);
}

TEST(TraitsTest, I16) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::I16);
  EXPECT_EQ(getName<tag>(), "i16");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<standard_type<tag>, int16_t>();
  IsSameType<standard_types<tag>, detail::types<int16_t, uint16_t>>();
  EXPECT_EQ(toTType(BaseType::I16), TType::T_I16);
  EXPECT_EQ(toThriftBaseType(TType::T_I16), BaseType::I16);
}

TEST(TraitsTest, I32) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::I32);
  EXPECT_EQ(getName<tag>(), "i32");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<standard_type<tag>, int32_t>();
  IsSameType<standard_types<tag>, detail::types<int32_t, uint32_t>>();
  EXPECT_EQ(toTType(BaseType::I32), TType::T_I32);
  EXPECT_EQ(toThriftBaseType(TType::T_I32), BaseType::I32);
}

TEST(TraitsTest, I64) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::I64);
  EXPECT_EQ(getName<tag>(), "i64");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<standard_type<tag>, int64_t>();
  IsSameType<standard_types<tag>, detail::types<int64_t, uint64_t>>();
  EXPECT_EQ(toTType(BaseType::I64), TType::T_I64);
  EXPECT_EQ(toThriftBaseType(TType::T_I64), BaseType::I64);
}

TEST(TraitsTest, Enum) {
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
  EXPECT_EQ(base_type_v<tag>, BaseType::Enum);
  EXPECT_EQ(getName<tag>(), "enum");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_t = type::enum_t<BaseType>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Enum);
  EXPECT_EQ(getName<tag_t>(), "type.BaseType");
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<standard_type<tag_t>, BaseType>();
  IsSameType<standard_types<tag_t>, detail::types<BaseType>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::Enum), TType::T_I32);
}

TEST(TraitsTest, Float) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Float);
  EXPECT_EQ(getName<tag>(), "float");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<standard_type<tag>, float>();
  IsSameType<standard_types<tag>, detail::types<float>>();
  EXPECT_EQ(toTType(BaseType::Float), TType::T_FLOAT);
  EXPECT_EQ(toThriftBaseType(TType::T_FLOAT), BaseType::Float);
}

TEST(TraitsTest, Double) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Double);
  EXPECT_EQ(getName<tag>(), "double");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<standard_type<tag>, double>();
  IsSameType<standard_types<tag>, detail::types<double>>();
  EXPECT_EQ(toTType(BaseType::Double), TType::T_DOUBLE);
  EXPECT_EQ(toThriftBaseType(TType::T_DOUBLE), BaseType::Double);
}

TEST(TraitsTest, String) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::String);
  EXPECT_EQ(getName<tag>(), "string");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<standard_type<tag>, std::string>();
  IsSameType<
      standard_types<tag>,
      detail::types<std::string, std::string_view, folly::StringPiece>>();
  EXPECT_EQ(toTType(BaseType::String), TType::T_UTF8);
  EXPECT_EQ(toThriftBaseType(TType::T_UTF8), BaseType::String);
}

TEST(TraitsTest, Binary) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Binary);
  EXPECT_EQ(getName<tag>(), "binary");
  EXPECT_TRUE(is_concrete_type_v<tag>);
  IsSameType<standard_type<tag>, std::string>();
  IsSameType<
      standard_types<tag>,
      detail::types<std::string, folly::IOBuf, folly::ByteRange>>();
  EXPECT_EQ(toTType(BaseType::Binary), TType::T_STRING);
  EXPECT_EQ(toThriftBaseType(TType::T_STRING), BaseType::Binary);
}

TEST(TraitsTest, Struct) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Struct);
  EXPECT_EQ(getName<tag>(), "struct");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_t = type::struct_t<Object>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Struct);
  EXPECT_EQ(getName<tag_t>(), "object.Object");
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<standard_type<tag_t>, Object>();
  IsSameType<standard_types<tag_t>, detail::types<Object>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::Struct), TType::T_STRUCT);
  EXPECT_EQ(toThriftBaseType(TType::T_STRUCT), BaseType::Struct);
}

TEST(TraitsTest, Union) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Union);
  EXPECT_EQ(getName<tag>(), "union");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_t = type::union_t<Value>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Union);
  EXPECT_EQ(getName<tag_t>(), "object.Value");
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<standard_type<tag_t>, Value>();
  IsSameType<standard_types<tag_t>, detail::types<Value>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::Union), TType::T_STRUCT);
}

TEST(TraitsTest, Exception) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Exception);
  EXPECT_EQ(getName<tag>(), "exception");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  EXPECT_EQ(toTType(BaseType::Exception), TType::T_STRUCT);

  // TODO(afuller): Add a test exception and test the concrete form.
}

TEST(TraitsTest, List) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::List);
  EXPECT_EQ(getName<tag>(), "list");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_c = type::list<type::struct_c>;
  EXPECT_EQ(base_type_v<tag_c>, BaseType::List);
  EXPECT_EQ(getName<tag_c>(), "list<struct>");
  EXPECT_FALSE(is_concrete_type_v<tag_c>);
  EXPECT_TRUE(all_types::contains<tag_c>);

  using tag_t = type::list<type::string_t>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::List);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  EXPECT_EQ(getName<tag_t>(), "list<string>");
  IsSameType<standard_type<tag_t>, std::vector<std::string>>();
  IsSameType<
      standard_types<tag_t>,
      detail::types<
          std::vector<std::string>,
          std::vector<std::string_view>,
          std::vector<folly::StringPiece>>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::List), TType::T_LIST);
  EXPECT_EQ(toThriftBaseType(TType::T_LIST), BaseType::List);
}

TEST(TraitsTest, Set) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Set);
  EXPECT_EQ(getName<tag>(), "set");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_c = type::set<type::struct_c>;
  EXPECT_EQ(base_type_v<tag_c>, BaseType::Set);
  EXPECT_EQ(getName<tag_c>(), "set<struct>");
  EXPECT_FALSE(is_concrete_type_v<tag_c>);
  EXPECT_TRUE(all_types::contains<tag_c>);

  using tag_t = type::set<type::string_t>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Set);
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  EXPECT_EQ(getName<tag_t>(), "set<string>");
  IsSameType<standard_type<tag_t>, std::set<std::string>>();
  IsSameType<
      standard_types<tag_t>,
      detail::types<
          std::set<std::string>,
          std::set<std::string_view>,
          std::set<folly::StringPiece>>>();
  EXPECT_TRUE(all_types::contains<tag_t>);
  EXPECT_EQ(toTType(BaseType::Set), TType::T_SET);
  EXPECT_EQ(toThriftBaseType(TType::T_SET), BaseType::Set);
}

TEST(TraitsTest, Map) {
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

  EXPECT_EQ(base_type_v<tag>, BaseType::Map);
  EXPECT_EQ(getName<tag>(), "map");
  EXPECT_FALSE(is_concrete_type_v<tag>);

  using tag_c = type::map<type::struct_c, type::struct_c>;
  EXPECT_EQ(base_type_v<tag_c>, BaseType::Map);
  EXPECT_EQ(getName<tag_c>(), "map<struct, struct>");
  EXPECT_FALSE(is_concrete_type_v<tag_c>);
  EXPECT_TRUE(all_types::contains<tag_c>);

  using tag_kc = type::map<type::struct_c, type::byte_t>;
  EXPECT_EQ(base_type_v<tag_kc>, BaseType::Map);
  EXPECT_EQ(getName<tag_kc>(), "map<struct, byte>");
  EXPECT_FALSE(is_concrete_type_v<tag_kc>);
  EXPECT_TRUE(all_types::contains<tag_kc>);

  using tag_vc = type::map<type::byte_t, type::struct_c>;
  EXPECT_EQ(base_type_v<tag_vc>, BaseType::Map);
  EXPECT_EQ(getName<tag_vc>(), "map<byte, struct>");
  EXPECT_FALSE(is_concrete_type_v<tag_vc>);
  EXPECT_TRUE(all_types::contains<tag_vc>);

  using tag_t = type::map<type::i16_t, type::i32_t>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Map);
  EXPECT_EQ(getName<tag_t>(), "map<i16, i32>");
  EXPECT_TRUE(is_concrete_type_v<tag_t>);
  IsSameType<standard_type<tag_t>, std::map<int16_t, int32_t>>();
  IsSameType<
      standard_types<tag_t>,
      detail::types<std::map<int16_t, int32_t>, std::map<int16_t, uint32_t>>>();
  EXPECT_TRUE(all_types::contains<tag_t>);

  EXPECT_EQ(toTType(BaseType::Map), TType::T_MAP);
  EXPECT_EQ(toThriftBaseType(TType::T_MAP), BaseType::Map);
}

TEST(TraitsTest, ConcreteType_Bound) {
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

TEST(TraitsTest, IsStandardType) {
  EXPECT_TRUE((is_standard_type<type::i32_t, int32_t>::value));
  EXPECT_TRUE((is_standard_type<type::i32_t, uint32_t>::value));
  EXPECT_FALSE((is_standard_type<type::i32_t, int16_t>::value));
}

} // namespace
} // namespace apache::thrift::type
