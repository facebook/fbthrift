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
#include <thrift/lib/cpp2/type/Testing.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/thrift/gen-cpp2/type_fatal_all.h>
#include <thrift/lib/thrift/gen-cpp2/type_types.h>

namespace apache::thrift::type {
namespace {
using conformance::Object;
using conformance::Value;

template <typename Types, typename Tag, bool Expected>
void testContains() {
  static_assert(Types::template contains<Tag>() == Expected);
  if constexpr (
      is_concrete_v<Tag> &&
      // TODO(afuller): Support concrete named types.
      !named_types::contains<Tag>()) {
    EXPECT_EQ(Types::contains(AnyType::create<Tag>().base_type()), Expected);
  }
}

TEST(TraitsTest, Bool) {
  using tag = bool_t;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Bool);
  EXPECT_EQ(getName<tag>(), "bool");
  test::same_type<standard_type<tag>, bool>;
  test::same_type<native_type<tag>, bool>;
}

TEST(TraitsTest, Byte) {
  using tag = byte_t;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Byte);
  EXPECT_EQ(getName<tag>(), "byte");
  test::same_type<standard_type<tag>, int8_t>;
  test::same_type<native_type<tag>, int8_t>;
}

TEST(TraitsTest, I16) {
  using tag = i16_t;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::I16);
  EXPECT_EQ(getName<tag>(), "i16");
  test::same_type<standard_type<tag>, int16_t>;
  test::same_type<native_type<tag>, int16_t>;
}

TEST(TraitsTest, I32) {
  using tag = i32_t;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::I32);
  EXPECT_EQ(getName<tag>(), "i32");
  test::same_type<standard_type<tag>, int32_t>;
  test::same_type<native_type<tag>, int32_t>;
}

TEST(TraitsTest, I64) {
  using tag = i64_t;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::I64);
  EXPECT_EQ(getName<tag>(), "i64");
  test::same_type<standard_type<tag>, int64_t>;
  test::same_type<native_type<tag>, int64_t>;
}

TEST(TraitsTest, Enum) {
  using tag = enum_c;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Enum);
  EXPECT_EQ(getName<tag>(), "enum");

  using tag_t = enum_t<ThriftBaseType>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Enum);
  EXPECT_EQ(getName<tag_t>(), "type.BaseType");
  test::same_type<standard_type<tag_t>, ThriftBaseType>;
  test::same_type<native_type<tag_t>, ThriftBaseType>;

  using tag_at = adapted<StaticCastAdapter<BaseType, ThriftBaseType>, tag_t>;
  EXPECT_EQ(base_type_v<tag_at>, BaseType::Enum);
  EXPECT_EQ(getName<tag_at>(), "apache::thrift::type::BaseType");
  test::same_type<standard_type<tag_at>, ThriftBaseType>;
  test::same_type<native_type<tag_at>, BaseType>;
}

TEST(TraitsTest, Float) {
  using tag = float_t;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Float);
  EXPECT_EQ(getName<tag>(), "float");
  test::same_type<standard_type<tag>, float>;
  test::same_type<native_type<tag>, float>;
}

TEST(TraitsTest, Double) {
  using tag = double_t;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Double);
  EXPECT_EQ(getName<tag>(), "double");
  test::same_type<standard_type<tag>, double>;
  test::same_type<native_type<tag>, double>;
}

TEST(TraitsTest, String) {
  using tag = string_t;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::String);
  EXPECT_EQ(getName<tag>(), "string");
  test::same_type<standard_type<tag>, std::string>;
  test::same_type<native_type<tag>, std::string>;
}

TEST(TraitsTest, Binary) {
  using tag = binary_t;
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Binary);
  EXPECT_EQ(getName<tag>(), "binary");
  test::same_type<standard_type<tag>, std::string>;
  test::same_type<native_type<tag>, std::string>;
}

TEST(TraitsTest, Struct) {
  using tag = struct_c;
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, true>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Struct);
  EXPECT_EQ(getName<tag>(), "struct");

  using tag_t = struct_t<Object>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Struct);
  EXPECT_EQ(getName<tag_t>(), "object.Object");
  test::same_type<standard_type<tag_t>, Object>;
  test::same_type<native_type<tag_t>, Object>;
}

TEST(TraitsTest, Union) {
  using tag = union_c;
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, true>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Union);
  EXPECT_EQ(getName<tag>(), "union");

  using tag_t = union_t<Value>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Union);
  EXPECT_EQ(getName<tag_t>(), "object.Value");
  test::same_type<standard_type<tag_t>, Value>;
  test::same_type<native_type<tag_t>, Value>;
}

TEST(TraitsTest, Exception) {
  using tag = exception_c;
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, true>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Exception);
  EXPECT_EQ(getName<tag>(), "exception");

  // TODO(afuller): Add a test exception and test the concrete form.
}

TEST(TraitsTest, List) {
  using tag = list_c;
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, false>();
  testContains<container_types, tag, true>();
  testContains<composite_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::List);
  EXPECT_EQ(getName<tag>(), "list");

  using tag_c = list<struct_c>;
  EXPECT_EQ(base_type_v<tag_c>, BaseType::List);
  EXPECT_EQ(getName<tag_c>(), "list<struct>");

  using tag_t = list<string_t>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::List);
  EXPECT_EQ(getName<tag_t>(), "list<string>");
  test::same_type<standard_type<tag_t>, std::vector<std::string>>;
  test::same_type<native_type<tag_t>, std::vector<std::string>>;
}

TEST(TraitsTest, Set) {
  using tag = set_c;
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, false>();
  testContains<container_types, tag, true>();
  testContains<composite_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Set);
  EXPECT_EQ(getName<tag>(), "set");

  using tag_c = set<struct_c>;
  EXPECT_EQ(base_type_v<tag_c>, BaseType::Set);
  EXPECT_EQ(getName<tag_c>(), "set<struct>");

  using tag_t = set<string_t>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Set);
  EXPECT_EQ(getName<tag_t>(), "set<string>");
  test::same_type<standard_type<tag_t>, std::set<std::string>>;
  test::same_type<native_type<tag_t>, std::set<std::string>>;
}

TEST(TraitsTest, Map) {
  using tag = map_c;
  testContains<primitive_types, tag, false>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, false>();
  testContains<container_types, tag, true>();
  testContains<composite_types, tag, true>();

  EXPECT_EQ(base_type_v<tag>, BaseType::Map);
  EXPECT_EQ(getName<tag>(), "map");

  using tag_c = map<struct_c, struct_c>;
  EXPECT_EQ(base_type_v<tag_c>, BaseType::Map);
  EXPECT_EQ(getName<tag_c>(), "map<struct, struct>");

  using tag_kc = map<struct_c, byte_t>;
  EXPECT_EQ(base_type_v<tag_kc>, BaseType::Map);
  EXPECT_EQ(getName<tag_kc>(), "map<struct, byte>");

  using tag_vc = map<byte_t, struct_c>;
  EXPECT_EQ(base_type_v<tag_vc>, BaseType::Map);
  EXPECT_EQ(getName<tag_vc>(), "map<byte, struct>");

  using tag_t = map<i16_t, i32_t>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::Map);
  EXPECT_EQ(getName<tag_t>(), "map<i16, i32>");
  test::same_type<standard_type<tag_t>, std::map<int16_t, int32_t>>;
  test::same_type<native_type<tag_t>, std::map<int16_t, int32_t>>;
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
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  EXPECT_EQ(base_type_v<tag>, BaseType::I64);
  test::same_type<standard_type<tag>, int64_t>;

  // The name and native_type have changed.
  EXPECT_EQ(getName<tag>(), folly::pretty_name<TestValue<long>>());
  test::same_type<native_type<tag>, TestValue<int64_t>>;
}

TEST(TraitsTest, CppType) {
  using tag = cpp_type<uint64_t, i64_t>;
  // All traits that operate on the standard type, match the given tag.
  testContains<primitive_types, tag, true>();
  testContains<structured_types, tag, false>();
  testContains<singular_types, tag, true>();
  testContains<container_types, tag, false>();
  testContains<composite_types, tag, false>();
  EXPECT_EQ(base_type_v<tag>, BaseType::I64);
  test::same_type<standard_type<tag>, int64_t>;

  // The name and native_type have changed.
  EXPECT_EQ(getName<tag>(), folly::pretty_name<uint64_t>());
  test::same_type<native_type<tag>, uint64_t>;
}

TEST(TraitsTest, AdaptedListElems) {
  using tag_t = list<adapted<TestAdapter, i64_t>>;
  EXPECT_EQ(base_type_v<tag_t>, BaseType::List);
  test::same_type<standard_type<tag_t>, std::vector<int64_t>>;

  EXPECT_EQ(
      getName<tag_t>(),
      fmt::format("list<{}>", folly::pretty_name<TestValue<long>>()));
  test::same_type<native_type<tag_t>, std::vector<TestValue<int64_t>>>;
}

TEST(TraitsTest, Filter) {
  test::same_type<
      primitive_types::filter<bound::is_concrete>,
      detail::types<
          bool_t,
          byte_t,
          i16_t,
          i32_t,
          i64_t,
          float_t,
          double_t,
          string_t,
          binary_t>>;
}

} // namespace
} // namespace apache::thrift::type
