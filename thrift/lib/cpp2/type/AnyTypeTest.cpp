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

#include <thrift/lib/cpp2/type/AnyType.h>

#include <list>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/type/Traits.h>
#include <thrift/lib/thrift/gen-cpp2/type_types.h>

namespace apache::thrift::type {
namespace {

struct AnyTypeTestCase {
  AnyType type;
  BaseType expected_type;
};

template <typename Tag, typename... Args>
AnyTypeTestCase test(Args&&... args) {
  return {AnyType::create<Tag>(std::forward<Args>(args)...), base_type_v<Tag>};
}

std::vector<AnyTypeTestCase> getUniqueNonContainerTypes() {
  return {
      test<void_t>(),
      test<bool_t>(),
      test<byte_t>(),
      test<i16_t>(),
      test<i32_t>(),
      test<i64_t>(),
      test<float_t>(),
      test<double_t>(),
      test<string_t>(),
      test<binary_t>(),
      test<struct_c>("d.c/p/MyStruct"),
      test<struct_c>("d.c/p/MyOtherStruct"),
      test<struct_c>("d.c/p/MyNamed"),
      test<union_c>("d.c/p/MyUnion"),
      test<union_c>("d.c/p/MyOtherUnion"),
      test<union_c>("d.c/p/MyNamed"),
      test<exception_c>("d.c/p/MyException"),
      test<exception_c>("d.c/p/MyOtherExcpetion"),
      test<exception_c>("d.c/p/MyNamed"),
      test<enum_c>("d.c/p/MyEnum"),
      test<enum_c>("d.c/p/MyOtherEnum"),
      test<enum_c>("d.c/p/MyNamed"),
  };
}

std::vector<AnyTypeTestCase> getUniqueContainerTypesOf(
    const std::vector<AnyTypeTestCase>& keys,
    const std::vector<AnyTypeTestCase>& values) {
  std::vector<AnyTypeTestCase> result;
  for (const auto& val : values) {
    result.emplace_back(test<list_c>(val.type));
  }
  for (const auto& key : keys) {
    result.emplace_back(test<set_c>(key.type));
    for (const auto& val : values) {
      result.emplace_back(test<map_c>(key.type, val.type));
    }
  }
  return result;
}

std::vector<AnyTypeTestCase> getUniqueTypes() {
  auto unique = getUniqueNonContainerTypes();
  auto uniqueContainer = getUniqueContainerTypesOf(unique, unique);
  unique.insert(unique.end(), uniqueContainer.begin(), uniqueContainer.end());
  return unique;
}

TEST(AnyTypeTest, Equality) {
  auto unique = getUniqueTypes();
  EXPECT_FALSE(unique.empty());
  for (size_t i = 0; i < unique.size(); ++i) {
    for (size_t j = 0; j < unique.size(); ++j) {
      EXPECT_EQ(unique[i].type == unique[j].type, i == j);
      EXPECT_EQ(unique[i].type != unique[j].type, i != j);
    }
  }
}

TEST(AnyTypeTest, BaseType) {
  auto unique = getUniqueTypes();
  for (const auto& test : unique) {
    EXPECT_EQ(test.type.base_type(), test.expected_type);
  }
}

struct MyStruct {
  constexpr static auto __fbthrift_cpp2_gen_thrift_uri() {
    return "domain.com/my/package/MyStruct";
  }
};

struct MyAdapter {};

TEST(AnyTypeTest, Create) {
  EXPECT_EQ(
      AnyType::create<list<i16_t>>(),
      AnyType::create<list_c>(AnyType::create<i16_t>()));

  EXPECT_EQ(
      AnyType::create<set<i16_t>>(),
      AnyType::create<set_c>(AnyType::create<i16_t>()));
  EXPECT_EQ(
      AnyType::create<set<list<i16_t>>>(),
      AnyType::create<set_c>(AnyType::create<list<i16_t>>()));

  EXPECT_EQ(
      (AnyType::create<map<string_t, binary_t>>()),
      AnyType::create<map_c>(
          AnyType::create<string_t>(), AnyType::create<binary_t>()));
  EXPECT_EQ(
      (AnyType::create<map<set<string_t>, list<binary_t>>>()),
      AnyType::create<map_c>(
          AnyType::create<set<string_t>>(), AnyType::create<list<binary_t>>()));
  EXPECT_EQ(
      AnyType::create<struct_t<MyStruct>>(),
      AnyType::create<struct_c>("domain.com/my/package/MyStruct"));
  EXPECT_EQ(
      AnyType::create<union_t<MyStruct>>(),
      AnyType::create<union_c>("domain.com/my/package/MyStruct"));
  EXPECT_EQ(
      AnyType::create<exception_t<MyStruct>>(),
      AnyType::create<exception_c>("domain.com/my/package/MyStruct"));
}

TEST(AnyTypeTest, Adapted) {
  // Adapted is ignored.
  EXPECT_EQ(
      (AnyType::create<adapted<MyAdapter, void_t>>()),
      AnyType::create<void_t>());
}

TEST(AnyTypeTest, CustomContainer) {
  // Custom container type is ignored.
  EXPECT_EQ(
      (AnyType::create<list<void_t, std::list>>()),
      AnyType::create<list<void_t>>());
  EXPECT_EQ(
      (AnyType::create<set<void_t, std::unordered_set>>()),
      AnyType::create<set<void_t>>());
  EXPECT_EQ(
      (AnyType::create<map<void_t, void_t, std::unordered_set>>()),
      (AnyType::create<map<void_t, void_t>>()));
}

TEST(AnyTypeTest, ImplicitConversion) {
  EXPECT_EQ(list<i16_t>{}, AnyType::create<list_c>(i16_t{}));
  AnyType type = i16_t{};
  EXPECT_EQ(type, AnyType::create<i16_t>());
  type = void_t{};
  EXPECT_NE(type, AnyType::create<i16_t>());
  EXPECT_EQ(type, AnyType());
}

TEST(AnyTypeTest, NameValidation) {
  EXPECT_THROW(AnyType::create<enum_c>("BadName"), std::invalid_argument);
  EXPECT_THROW(AnyType::create<struct_c>("BadName"), std::invalid_argument);
  EXPECT_THROW(AnyType::create<union_c>("BadName"), std::invalid_argument);
  EXPECT_THROW(AnyType::create<exception_c>("BadName"), std::invalid_argument);
}

} // namespace
} // namespace apache::thrift::type
