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

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp/Field.h>
#include <thrift/lib/cpp2/gen/module_types_h.h>
#include <thrift/lib/cpp2/op/Create.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/Testing.h>
#include <thrift/lib/thrift/gen-cpp2/type_types.h>
#include <thrift/test/testset/Testset.h>
#include <thrift/test/testset/gen-cpp2/testset_types.h>

namespace apache::thrift::op {
namespace {

using namespace apache::thrift::type;
namespace testset = apache::thrift::test::testset;

// Wrapper with default constructor deleted.
template <typename T>
struct Wrapper {
  T value;
  // TODO(afuller): Support adapting the 'create' op.
  // Wrapper() = delete;
  bool operator==(const Wrapper& other) const { return value == other.value; }
  bool operator<(const Wrapper& other) const { return value < other.value; }
};

// Wrapper with context with default constructor deleted.
template <typename T>
struct WrapperWithContext {
  T value;
  int16_t fieldId = 0;
  std::string* meta = nullptr;
  // TODO(afuller): Support adapting the 'create' op.
  // WrapperWithContext() = delete;
  bool operator==(const WrapperWithContext& other) const {
    return value == other.value;
  }
  bool operator<(const WrapperWithContext& other) const {
    return value < other.value;
  }
};

struct TestTypeAdapter {
  template <typename T>
  static Wrapper<T> fromThrift(T value) {
    return {value};
  }

  template <typename T>
  static T toThrift(Wrapper<T> wrapper) {
    return wrapper.value;
  }
};

struct TestFieldAdapter {
  template <typename T, typename Struct, int16_t FieldId>
  static WrapperWithContext<T> fromThriftField(
      T value, apache::thrift::FieldContext<Struct, FieldId>&& ctx) {
    return {value, ctx.kFieldId, &ctx.object.meta};
  }

  template <typename T>
  static T toThrift(WrapperWithContext<T> wrapper) {
    return wrapper.value;
  }

  template <typename T, typename Context>
  static void construct(WrapperWithContext<T>& field, Context&& ctx) {
    field.fieldId = Context::kFieldId;
    field.meta = &ctx.object.meta;
  }
};

struct TestThriftType {
  std::string meta;
};

template <typename Tag>
void testCreateWithTag() {
  using tag = Tag;
  using field_tag = field_t<FieldId{0}, tag>;
  using adapted_tag = adapted<TestTypeAdapter, tag>;
  using type_adapted_field_tag = field_t<FieldId{0}, adapted_tag>;
  using field_adapted_field_tag =
      field_t<FieldId{0}, adapted<TestFieldAdapter, tag>>;

  TestThriftType object;

  auto type_created = create<tag>();
  auto adapted_created = create<adapted_tag>();
  auto field_created = create<field_tag>(object);
  auto type_adapted_field_created = create<type_adapted_field_tag>(object);
  auto field_adapted_field_created = create<field_adapted_field_tag>(object);

  test::same_type<decltype(type_created), native_type<tag>>;
  test::same_type<decltype(adapted_created), Wrapper<native_type<tag>>>;
  test::same_type<decltype(field_created), native_type<tag>>;
  test::same_type<
      decltype(type_adapted_field_created),
      Wrapper<native_type<tag>>>;
  test::same_type<
      decltype(field_adapted_field_created),
      WrapperWithContext<native_type<tag>>>;

  Wrapper<native_type<tag>> type_adapted_default{native_type<tag>()};
  WrapperWithContext<native_type<tag>> field_adapted_default{
      native_type<tag>()};

  EXPECT_EQ(type_created, native_type<tag>());
  EXPECT_EQ(adapted_created, type_adapted_default);
  EXPECT_EQ(field_created, native_type<tag>());
  EXPECT_EQ(type_adapted_field_created, type_adapted_default);
  EXPECT_EQ(field_adapted_field_created, field_adapted_default);
  // Check if the context is correctly populated.
  EXPECT_EQ(&object.meta, field_adapted_field_created.meta);
}

template <typename Tag>
void testCreateStructured() {
  testCreateWithTag<struct_t<testset::struct_with<Tag>>>();
  testCreateWithTag<
      struct_t<testset::struct_with<Tag, testset::FieldModifier::Optional>>>();
  testCreateWithTag<exception_t<testset::exception_with<Tag>>>();
  testCreateWithTag<exception_t<
      testset::exception_with<Tag, testset::FieldModifier::Optional>>>();
  testCreateWithTag<union_t<testset::union_with<Tag>>>();
}

template <typename Tag>
void testCreate() {
  testCreateWithTag<Tag>();
  testCreateStructured<Tag>();
}

TEST(CreateTest, Integral) {
  testCreate<bool_t>();
  testCreate<byte_t>();
  testCreate<i16_t>();
  testCreate<i32_t>();
  testCreate<i64_t>();
  // testset does not include structured with Enum.
  testCreateWithTag<enum_t<BaseTypeEnum>>();
}

TEST(CreateTest, FloatingPoint) {
  testCreate<float_t>();
  testCreate<double_t>();
}

TEST(CreateTest, String) {
  testCreate<string_t>();
  testCreate<binary_t>();
}

TEST(CreateTest, Container) {
  testCreate<list<string_t>>();
  testCreate<set<string_t>>();
  testCreate<map<string_t, string_t>>();
}

} // namespace
} // namespace apache::thrift::op
