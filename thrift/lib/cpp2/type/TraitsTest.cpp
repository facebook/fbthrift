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

#include <list>
#include <unordered_map>
#include <unordered_set>

#include <thrift/lib/cpp2/type/Traits.h>

#include <folly/portability/GTest.h>
#include <thrift/conformance/if/gen-cpp2/object_fatal_all.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>
#include <thrift/lib/cpp2/type/AnyType.h>
#include <thrift/lib/cpp2/type/BaseType.h>
#include <thrift/lib/cpp2/type/Name.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/thrift/gen-cpp2/type_fatal_all.h>
#include <thrift/lib/thrift/gen-cpp2/type_types.h>

namespace apache::thrift::type {
namespace {
using conformance::Object;
using conformance::Value;

template <typename actual, typename expected>
struct IsSameType;

template <typename T>
struct IsSameType<T, T> {};

template <typename Types, typename Tag, bool Expected>
void testContains() {
  static_assert(Types::template contains<Tag>() == Expected);
  if constexpr (
      is_concrete_v<Tag> &&
      // TODO(afuller): Support concrete named types.
      !named_types::contains<Tag>()) {
    EXPECT_EQ(Types::contains(AnyType::create<Tag>()), Expected);
  }
}

TEST(TraitsTest, Bool) {
  using tag = bool_t;
  testContains<integral_types, tag, true>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, true>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Bool);
  EXPECT_EQ(getName<tag>(), "bool");
  IsSameType<standard_type<tag>, bool>();
  IsSameType<native_type<tag>, bool>();
  IsSameType<standard_types<tag>, detail::types<bool>>();
}

TEST(TraitsTest, Byte) {
  using tag = byte_t;
  testContains<integral_types, tag, true>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, true>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Byte);
  EXPECT_EQ(getName<tag>(), "byte");
  IsSameType<standard_type<tag>, int8_t>();
  IsSameType<native_type<tag>, int8_t>();
  IsSameType<standard_types<tag>, detail::types<int8_t, uint8_t>>();
}

TEST(TraitsTest, I16) {
  using tag = i16_t;
  testContains<integral_types, tag, true>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, true>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::I16);
  EXPECT_EQ(getName<tag>(), "i16");
  IsSameType<standard_type<tag>, int16_t>();
  IsSameType<native_type<tag>, int16_t>();
  IsSameType<standard_types<tag>, detail::types<int16_t, uint16_t>>();
}

TEST(TraitsTest, I32) {
  using tag = i32_t;
  testContains<integral_types, tag, true>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, true>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::I32);
  EXPECT_EQ(getName<tag>(), "i32");
  IsSameType<standard_type<tag>, int32_t>();
  IsSameType<native_type<tag>, int32_t>();
  IsSameType<standard_types<tag>, detail::types<int32_t, uint32_t>>();
}

TEST(TraitsTest, I64) {
  using tag = i64_t;
  testContains<integral_types, tag, true>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, true>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::I64);
  EXPECT_EQ(getName<tag>(), "i64");
  IsSameType<standard_type<tag>, int64_t>();
  IsSameType<native_type<tag>, int64_t>();
  IsSameType<standard_types<tag>, detail::types<int64_t, uint64_t>>();
}

TEST(TraitsTest, Enum) {
  using tag = enum_c;
  testContains<integral_types, tag, true>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, true>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Enum);
  EXPECT_EQ(getName<tag>(), "enum");

  using tag_t = enum_t<ThriftBaseType>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Enum);
  EXPECT_EQ(getName<tag_t>(), "type.BaseType");
  IsSameType<standard_type<tag_t>, ThriftBaseType>();
  IsSameType<native_type<tag_t>, ThriftBaseType>();
  IsSameType<standard_types<tag_t>, detail::types<ThriftBaseType>>();
  testContains<all_types, tag_t, true>();

  using tag_at = adapted<StaticCastAdapter<BaseType, ThriftBaseType>, tag_t>;
  EXPECT_EQ(base_type_v<tag_at>, BaseType::Enum);
  EXPECT_EQ(getName<tag_at>(), "apache::thrift::type::BaseType");
  IsSameType<standard_type<tag_at>, ThriftBaseType>();
  IsSameType<native_type<tag_at>, BaseType>();
  IsSameType<standard_types<tag_at>, detail::types<ThriftBaseType>>();
  testContains<all_types, tag_at, true>();
}

TEST(TraitsTest, Float) {
  using tag = float_t;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, true>();
  testContains<numeric_types, tag, true>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Float);
  EXPECT_EQ(getName<tag>(), "float");
  IsSameType<standard_type<tag>, float>();
  IsSameType<native_type<tag>, float>();
  IsSameType<standard_types<tag>, detail::types<float>>();
}

TEST(TraitsTest, Double) {
  using tag = double_t;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, true>();
  testContains<numeric_types, tag, true>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Double);
  EXPECT_EQ(getName<tag>(), "double");
  IsSameType<standard_type<tag>, double>();
  IsSameType<native_type<tag>, double>();
  IsSameType<standard_types<tag>, detail::types<double>>();
}

TEST(TraitsTest, String) {
  using tag = string_t;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, false>();
  testContains<string_types, tag, true>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::String);
  EXPECT_EQ(getName<tag>(), "string");
  IsSameType<standard_type<tag>, std::string>();
  IsSameType<native_type<tag>, std::string>();
  IsSameType<
      standard_types<tag>,
      detail::types<
          std::string,
          folly::fbstring,
          folly::IOBuf,
          std::unique_ptr<folly::IOBuf>>>();
}

TEST(TraitsTest, Binary) {
  using tag = binary_t;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, false>();
  testContains<string_types, tag, true>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Binary);
  EXPECT_EQ(getName<tag>(), "binary");
  IsSameType<standard_type<tag>, std::string>();
  IsSameType<native_type<tag>, std::string>();
  IsSameType<
      standard_types<tag>,
      detail::types<
          std::string,
          folly::fbstring,
          folly::IOBuf,
          std::unique_ptr<folly::IOBuf>>>();
}

TEST(TraitsTest, Struct) {
  using tag = struct_c;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, false>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, true>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, true>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Struct);
  EXPECT_EQ(getName<tag>(), "struct");

  using tag_t = struct_t<Object>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Struct);
  EXPECT_EQ(getName<tag_t>(), "object.Object");
  IsSameType<standard_type<tag_t>, Object>();
  IsSameType<native_type<tag_t>, Object>();
  IsSameType<standard_types<tag_t>, detail::types<Object>>();
  testContains<all_types, tag_t, true>();
}

TEST(TraitsTest, Union) {
  using tag = union_c;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, false>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, true>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, true>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Union);
  EXPECT_EQ(getName<tag>(), "union");

  using tag_t = union_t<Value>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Union);
  EXPECT_EQ(getName<tag_t>(), "object.Value");
  IsSameType<standard_type<tag_t>, Value>();
  IsSameType<native_type<tag_t>, Value>();
  IsSameType<standard_types<tag_t>, detail::types<Value>>();
  testContains<all_types, tag_t, true>();
}

TEST(TraitsTest, Exception) {
  using tag = exception_c;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, false>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, true>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, true>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Exception);
  EXPECT_EQ(getName<tag>(), "exception");

  // TODO(afuller): Add a test exception and test the concrete form.
}

TEST(TraitsTest, List) {
  using tag = list_c;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, false>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, false>();
  testContains<container_types, tag, true>();
  testContains<composite_types, tag, true>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::List);
  EXPECT_EQ(getName<tag>(), "list");

  using tag_c = list<struct_c>;
  EXPECT_EQ(base_type_v<tag_c>, BaseType::List);
  EXPECT_EQ(getName<tag_c>(), "list<struct>");
  testContains<all_types, tag_c, true>();

  using tag_t = list<string_t>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::List);
  EXPECT_EQ(getName<tag_t>(), "list<string>");
  IsSameType<standard_type<tag_t>, std::vector<std::string>>();
  IsSameType<native_type<tag_t>, std::vector<std::string>>();
  IsSameType<
      standard_types<tag_t>,
      detail::types<
          std::vector<std::string>,
          std::vector<folly::fbstring>,
          std::vector<folly::IOBuf>,
          std::vector<std::unique_ptr<folly::IOBuf>>>>();
  testContains<all_types, tag_t, true>();

  using tag_s = list<string_t, std::list>;
  EXPECT_EQ(base_type_v<tag_s>, BaseType::List);
  EXPECT_EQ(getName<tag_s>(), "list<string>");
  IsSameType<standard_type<tag_s>, std::list<std::string>>();
  IsSameType<native_type<tag_s>, std::list<std::string>>();
  IsSameType<
      standard_types<tag_s>,
      detail::types<
          std::list<std::string>,
          std::list<folly::fbstring>,
          std::list<folly::IOBuf>,
          std::list<std::unique_ptr<folly::IOBuf>>>>();
  testContains<all_types, tag_s, true>();
}

TEST(TraitsTest, Set) {
  using tag = set_c;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, false>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, false>();
  testContains<container_types, tag, true>();
  testContains<composite_types, tag, true>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Set);
  EXPECT_EQ(getName<tag>(), "set");

  using tag_c = set<struct_c>;
  EXPECT_EQ(base_type_v<tag_c>, BaseType::Set);
  EXPECT_EQ(getName<tag_c>(), "set<struct>");
  testContains<all_types, tag_c, true>();

  using tag_t = set<string_t>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Set);
  EXPECT_EQ(getName<tag_t>(), "set<string>");
  IsSameType<standard_type<tag_t>, std::set<std::string>>();
  IsSameType<native_type<tag_t>, std::set<std::string>>();
  IsSameType<
      standard_types<tag_t>,
      detail::types<
          std::set<std::string>,
          std::set<folly::fbstring>,
          std::set<folly::IOBuf>,
          std::set<std::unique_ptr<folly::IOBuf>>>>();
  testContains<all_types, tag_t, true>();

  using tag_s = set<string_t, std::unordered_set>;
  EXPECT_EQ(base_type_v<tag_s>, BaseType::Set);
  EXPECT_EQ(getName<tag_s>(), "set<string>");
  IsSameType<standard_type<tag_s>, std::unordered_set<std::string>>();
  IsSameType<native_type<tag_s>, std::unordered_set<std::string>>();
  IsSameType<
      standard_types<tag_s>,
      detail::types<
          std::unordered_set<std::string>,
          std::unordered_set<folly::fbstring>,
          std::unordered_set<folly::IOBuf>,
          std::unordered_set<std::unique_ptr<folly::IOBuf>>>>();
  testContains<all_types, tag_s, true>();
}

TEST(TraitsTest, Map) {
  using tag = map_c;
  testContains<integral_types, tag, false>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, false>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, false>();
  testContains<container_types, tag, true>();
  testContains<composite_types, tag, true>();
  testContains<all_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Map);
  EXPECT_EQ(getName<tag>(), "map");

  using tag_c = map<struct_c, struct_c>;
  EXPECT_EQ(base_type_v<tag_c>, BaseType::Map);
  EXPECT_EQ(getName<tag_c>(), "map<struct, struct>");
  testContains<all_types, tag_c, true>();

  using tag_kc = map<struct_c, byte_t>;
  EXPECT_EQ(base_type_v<tag_kc>, BaseType::Map);
  EXPECT_EQ(getName<tag_kc>(), "map<struct, byte>");
  testContains<all_types, tag_kc, true>();

  using tag_vc = map<byte_t, struct_c>;
  EXPECT_EQ(base_type_v<tag_vc>, BaseType::Map);
  EXPECT_EQ(getName<tag_vc>(), "map<byte, struct>");
  testContains<all_types, tag_vc, true>();

  using tag_t = map<i16_t, i32_t>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Map);
  EXPECT_EQ(getName<tag_t>(), "map<i16, i32>");
  IsSameType<standard_type<tag_t>, std::map<int16_t, int32_t>>();
  IsSameType<native_type<tag_t>, std::map<int16_t, int32_t>>();
  IsSameType<
      standard_types<tag_t>,
      detail::types<std::map<int16_t, int32_t>, std::map<int16_t, uint32_t>>>();
  testContains<all_types, tag_t, true>();

  using tag_s = map<i16_t, i32_t, std::unordered_map>;
  EXPECT_EQ(base_type_v<tag_s>, BaseType::Map);
  EXPECT_EQ(getName<tag_s>(), "map<i16, i32>");
  IsSameType<standard_type<tag_s>, std::unordered_map<int16_t, int32_t>>();
  IsSameType<native_type<tag_s>, std::unordered_map<int16_t, int32_t>>();
  IsSameType<
      standard_types<tag_s>,
      detail::types<
          std::unordered_map<int16_t, int32_t>,
          std::unordered_map<int16_t, uint32_t>>>();
  testContains<all_types, tag_s, true>();
}

template <typename T>
struct TestValue {
  T value;
};

struct TestAdapter {
  template <typename T>
  static TestValue<T> fromThrift(T&& value) {
    return {std::forward<T>(value)};
  }
};

TEST(TraitsTest, Adapted) {
  using tag = adapted<TestAdapter, i64_t>;
  // All traits that operate on the standard type, match the given tag.
  testContains<integral_types, tag, true>();
  testContains<floating_point_types, tag, false>();
  testContains<numeric_types, tag, true>();
  testContains<string_types, tag, false>();
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  testContains<all_types, tag, true>();
  EXPECT_EQ(base_type_v<tag>, BaseType::I64);
  IsSameType<standard_type<tag>, int64_t>();
  IsSameType<standard_types<tag>, detail::types<int64_t, uint64_t>>();

  // The name and native_type have changed.
  EXPECT_EQ(getName<tag>(), folly::pretty_name<TestValue<long>>());
  IsSameType<native_type<tag>, TestValue<int64_t>>();
}

TEST(TraitsTest, AdaptedListElems) {
  using tag_t = list<adapted<TestAdapter, i64_t>>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::List);
  IsSameType<standard_type<tag_t>, std::vector<int64_t>>();
  testContains<all_types, tag_t, true>();

  EXPECT_EQ(
      getName<tag_t>(),
      fmt::format("list<{}>", folly::pretty_name<TestValue<long>>()));
  IsSameType<native_type<tag_t>, std::vector<TestValue<int64_t>>>();
}

TEST(TraitsTest, AdaptedSetEntries) {
  using tag_t = set<adapted<TestAdapter, i64_t>>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Set);
  IsSameType<standard_type<tag_t>, std::set<int64_t>>();
  testContains<all_types, tag_t, true>();

  EXPECT_EQ(
      getName<tag_t>(),
      fmt::format("set<{}>", folly::pretty_name<TestValue<long>>()));
  IsSameType<
      native_type<tag_t>,
      std::set<
          TestValue<int64_t>,
          adapt_detail::adapted_less<TestAdapter, TestValue<int64_t>>>>();
  IsSameType<
      native_type<set<adapted<TestAdapter, i64_t>, std::unordered_set>>,
      std::unordered_set<
          TestValue<int64_t>,
          adapt_detail::adapted_hash<TestAdapter, TestValue<int64_t>>,
          adapt_detail::adapted_equal<TestAdapter, TestValue<int64_t>>>>();
}

TEST(TraitsTest, AdaptedMapEntries) {
  using tag_t = map<adapted<TestAdapter, i64_t>, adapted<TestAdapter, bool_t>>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Map);
  IsSameType<standard_type<tag_t>, std::map<int64_t, bool>>();
  testContains<all_types, tag_t, true>();

  EXPECT_EQ(
      getName<tag_t>(),
      fmt::format(
          "map<{}, {}>",
          folly::pretty_name<TestValue<long>>(),
          folly::pretty_name<TestValue<bool>>()));
  IsSameType<
      native_type<tag_t>,
      std::map<
          TestValue<long>,
          TestValue<bool>,
          adapt_detail::adapted_less<TestAdapter, TestValue<long>>>>();
  IsSameType<
      native_type<
          map<adapted<TestAdapter, i64_t>,
              adapted<TestAdapter, bool_t>,
              std::unordered_map>>,
      std::unordered_map<
          TestValue<long>,
          TestValue<bool>,
          adapt_detail::adapted_hash<TestAdapter, TestValue<long>>,
          adapt_detail::adapted_equal<TestAdapter, TestValue<long>>>>();
}

TEST(TraitsTest, ConcreteType_Bound) {
  IsSameType<
      fatal::filter<all_types, bound::is_concrete>,
      detail::types<
          bool_t,
          byte_t,
          i16_t,
          i32_t,
          i64_t,
          float_t,
          double_t,
          string_t,
          binary_t>>();
}

TEST(TraitsTest, IsStandardType) {
  EXPECT_TRUE((is_standard_type<i32_t, int32_t>::value));
  EXPECT_TRUE((is_standard_type<i32_t, uint32_t>::value));
  EXPECT_FALSE((is_standard_type<i32_t, int16_t>::value));
}

} // namespace
} // namespace apache::thrift::type
