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
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <folly/portability/GTest.h>

namespace apache::thrift::type {
namespace {

std::vector<AnyType> getUniqueNonContainerTypes() {
  return {
      {void_t{}},
      {bool_t{}},
      {byte_t{}},
      {i16_t{}},
      {i32_t{}},
      {i64_t{}},
      {float_t{}},
      {double_t{}},
      {string_t{}},
      {binary_t{}},
      {struct_c{}, "MyStruct"},
      {struct_c{}, "MyOtherStruct"},
      {struct_c{}, "MyNamed"},
      {union_c{}, "MyUnion"},
      {union_c{}, "MyOtherUnion"},
      {union_c{}, "MyNamed"},
      {exception_c{}, "MyException"},
      {exception_c{}, "MyOtherExcpetion"},
      {exception_c{}, "MyNamed"},
      {enum_c{}, "MyEnum"},
      {enum_c{}, "MyOtherEnum"},
      {enum_c{}, "MyNamed"},
  };
}

std::vector<AnyType> getUniqueContainerTypesOf(
    const std::vector<AnyType>& keyTypes,
    const std::vector<AnyType>& valueTypes) {
  std::vector<AnyType> result;
  for (const auto& valType : valueTypes) {
    result.emplace_back(list_c{}, valType);
  }
  for (const auto& keyType : keyTypes) {
    result.emplace_back(set_c{}, keyType);
    for (const auto& valType : valueTypes) {
      result.emplace_back(map_c{}, keyType, valType);
    }
  }
  return result;
}

std::vector<AnyType> getUniqueTypes() {
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
      EXPECT_EQ(unique[i] == unique[j], i == j);
      EXPECT_EQ(unique[i] != unique[j], i != j);
    }
  }
}

struct MyStruct {};
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

  // TODO(afuller): support extracting names from concrete type tags for named
  // types.
  // Uncomment to get expected compile time error.
  // AnyType::create<struct_t<MyStruct>>();
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

} // namespace
} // namespace apache::thrift::type
