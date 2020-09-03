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

#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/protocol/NimbleProtocol.h>
#include <thrift/lib/cpp2/protocol/nimble/test/gen-cpp2/forward_compatibility_types.h>

using namespace apache::thrift::test;

namespace apache {
namespace thrift {
namespace detail {

Primitives defaultPrimitives() {
  Primitives result;
  *result.f1_ref() = 1;
  *result.f2_ref() = 2;
  *result.f3_ref() = 3;
  *result.f4_ref() = 4;
  *result.f5_ref() = "5";
  *result.f6_ref() = "6";
  *result.f7_ref() = "7";
  *result.f8_ref() = 8.0f;
  *result.f9_ref() = 9.0;
  return result;
}

BigFieldIds defaultBigFieldIds() {
  BigFieldIds result;
  *result.f1_ref() = 1;
  *result.f100_ref() = 100;
  *result.f2_ref() = 2;
  *result.f101_ref() = 101;
  *result.f102_ref() = 102;
  *result.f1000_ref() = 1000;
  *result.f1001_ref() = 1001;
  *result.f3_ref() = 3;
  *result.f4_ref() = 4;
  return result;
}

NestedStructL2 defaultNestedStructL2() {
  NestedStructL2 result;
  *result.f1_ref() = "1";
  *result.f2_ref()->f1_ref() = 1;
  *result.f2_ref()->f2_ref() = 2;
  *result.f2_ref()->f3_ref()->f1_ref() = 1;
  *result.f2_ref()->f3_ref()->f2_ref() = 2;
  *result.f3_ref() = 3;
  return result;
}

ListStructElem defaultListStructElem() {
  ListStructElem result;
  *result.f1_ref() = "1";
  *result.f2_ref() = 2;
  return result;
}

ListStruct defaultListStruct() {
  ListStruct result;
  *result.f1_ref() = std::vector<ListStructElem>(10, defaultListStructElem());
  *result.f2_ref()->f1_ref() = "unusual";
  *result.f2_ref()->f2_ref() = 123;
  *result.f3_ref() = 456;
  *result.f4_ref() = {1, 2, 3, 4, 5};
  *result.f5_ref() = 789;
  return result;
}

MapStruct defaultMapStruct() {
  MapStructValue mapValue;
  *mapValue.f1_ref() = "1";
  *mapValue.f2_ref() = 2;

  MapStruct result;
  *result.f1_ref() = {{1, mapValue}, {2, mapValue}, {3, mapValue}};
  *result.f2_ref()->f1_ref() = "abc";
  *result.f2_ref()->f2_ref() = 123;
  *result.f5_ref() = 5;
  return result;
}

template <typename Dst, typename Src>
Dst nimble_cast(Src& src) {
  NimbleProtocolWriter writer;
  src.write(&writer);
  auto buf = writer.finalize();
  buf->coalesce();

  NimbleProtocolReader reader;
  reader.setInput(folly::io::Cursor{buf.get()});
  Dst dst;
  dst.read(&reader);
  return dst;
}

TEST(NimbleForwardCompatibilityTest, PrimitiveSimpleSkip) {
  auto primitives = defaultPrimitives();
  auto casted = nimble_cast<PrimitivesSimpleSkip>(primitives);
  EXPECT_EQ(1, *casted.f1_ref());
  EXPECT_EQ(2, *casted.f2_ref());
  // Altered
  EXPECT_EQ(0, *casted.f3_ref());
  EXPECT_EQ(4, *casted.f4_ref());
  // Altered
  EXPECT_EQ("", *casted.f5_ref());
  EXPECT_EQ("6", *casted.f6_ref());
}

TEST(NimbleForwardCompatibilityTest, PrimitiveConsecutiveMissing) {
  auto primitives = defaultPrimitives();
  auto casted = nimble_cast<PrimitivesConsecutiveMissing>(primitives);

  EXPECT_EQ(1, *casted.f1_ref());
  EXPECT_EQ("6", *casted.f6_ref());
}

TEST(NimbleForwardCompatibilityTest, PrimitivesTypesChanged) {
  auto primitives = defaultPrimitives();
  auto casted = nimble_cast<PrimitivesTypesChanged>(primitives);

  // Altered
  EXPECT_EQ(0, *casted.f1_ref());
  EXPECT_EQ(2, *casted.f2_ref());
  // Altered
  EXPECT_EQ(0, *casted.f3_ref());
  EXPECT_EQ(4, *casted.f4_ref());
  // Altered
  EXPECT_EQ(0.0, *casted.f5_ref());
  // Altered
  EXPECT_TRUE(casted.f6_ref()->empty());
  EXPECT_EQ("7", *casted.f7_ref());
  EXPECT_EQ(8.0f, *casted.f8_ref());
  // Altered
  EXPECT_EQ(0.0, *casted.f9_ref());
}

TEST(NimbleForwardCompatibilityTest, PrimitivesTypesReordered) {
  auto primitives = defaultPrimitives();
  auto casted = nimble_cast<PrimitivesTypesReordered>(primitives);

  EXPECT_EQ(*primitives.f1_ref(), *casted.f1_ref());
  EXPECT_EQ(*primitives.f2_ref(), *casted.f2_ref());
  EXPECT_EQ(*primitives.f3_ref(), *casted.f3_ref());
  EXPECT_EQ(*primitives.f4_ref(), *casted.f4_ref());
  EXPECT_EQ(*primitives.f5_ref(), *casted.f5_ref());
  EXPECT_EQ(*primitives.f6_ref(), *casted.f6_ref());
  EXPECT_EQ(*primitives.f7_ref(), *casted.f7_ref());
  EXPECT_EQ(*primitives.f8_ref(), *casted.f8_ref());
  EXPECT_EQ(*primitives.f9_ref(), *casted.f9_ref());
}

TEST(NimbleForwardCompatibilityTest, BigFieldIds) {
  BigFieldIds bigFieldIds;
  *bigFieldIds.f1_ref() = 1;
  *bigFieldIds.f100_ref() = 100;
  *bigFieldIds.f2_ref() = 2;
  *bigFieldIds.f101_ref() = 101;
  *bigFieldIds.f102_ref() = 102;
  *bigFieldIds.f1000_ref() = 1000;
  *bigFieldIds.f1001_ref() = 1001;
  *bigFieldIds.f3_ref() = 3;
  *bigFieldIds.f4_ref() = 4;

  auto bigFieldIdsMissing = nimble_cast<BigFieldIdsMissing>(bigFieldIds);
  EXPECT_EQ(1, *bigFieldIdsMissing.f1_ref());
  EXPECT_EQ(2, *bigFieldIdsMissing.f2_ref());
  EXPECT_EQ(4, *bigFieldIdsMissing.f4_ref());
}

TEST(NimbleForwardCompatibilityTest, NestedStruct) {
  NestedStructL2 nested;
  *nested.f1_ref() = "1";
  *nested.f2_ref()->f1_ref() = 1;
  *nested.f2_ref()->f2_ref() = 2;
  *nested.f2_ref()->f3_ref()->f1_ref() = 1;
  *nested.f2_ref()->f3_ref()->f2_ref() = 2;
  *nested.f3_ref() = 3;

  auto casted = nimble_cast<NestedStructMissingSubstruct>(nested);
  EXPECT_EQ("1", *casted.f1_ref());
  EXPECT_EQ(3, *casted.f3_ref());
}

TEST(NimbleForwardCompatibilityTest, NestedStructTypeChanged) {
  auto nested = defaultNestedStructL2();
  auto casted = nimble_cast<NestedStructTypeChanged>(nested);
  EXPECT_EQ("1", *casted.f1_ref());
  EXPECT_EQ(3, *casted.f3_ref());
}

TEST(NimbleForwardCompatabilityTest, Lists) {
  auto listStruct = defaultListStruct();
  auto casted = nimble_cast<ListStructMissingFields>(listStruct);
  EXPECT_EQ("unusual", *casted.f2_ref()->f1_ref());
  EXPECT_EQ(123, *casted.f2_ref()->f2_ref());
  EXPECT_EQ(456, *casted.f3_ref());
  EXPECT_EQ(789, *casted.f5_ref());
}

TEST(NimbleForwardCompatibilityTest, ListTypeChanged) {
  auto listStruct = defaultListStruct();
  auto casted = nimble_cast<ListStructTypeChanged>(listStruct);
  EXPECT_EQ("unusual", *casted.f2_ref()->f1_ref());
  EXPECT_EQ(123, *casted.f2_ref()->f2_ref());
  EXPECT_EQ(456, *casted.f3_ref());
  std::vector<int32_t> expectedList = {1, 2, 3, 4, 5};
  EXPECT_EQ(expectedList, *casted.f4_ref());
  EXPECT_EQ(789, *casted.f5_ref());
}

TEST(NimbleForwardCompatibilityTest, ListElemTypeChanged) {
  auto listStruct = defaultListStruct();
  auto casted = nimble_cast<ListStructListElemTypeChanged>(listStruct);
  EXPECT_EQ("unusual", *casted.f2_ref()->f1_ref());
  EXPECT_EQ(123, *casted.f2_ref()->f2_ref());
  EXPECT_EQ(456, *casted.f3_ref());
  std::vector<int32_t> expectedList = {1, 2, 3, 4, 5};
  EXPECT_EQ(expectedList, *casted.f4_ref());
  EXPECT_EQ(789, *casted.f5_ref());
}

TEST(NimbleForwardCompatibilityTest, Maps) {
  auto mapStruct = defaultMapStruct();
  auto casted = nimble_cast<MapStructMissingFields>(mapStruct);
  EXPECT_EQ("abc", *casted.f2_ref()->f1_ref());
  EXPECT_EQ(123, *casted.f2_ref()->f2_ref());
}

TEST(NimbleForwardCompatibilityTest, MapTypeChanged) {
  auto mapStruct = defaultMapStruct();
  auto casted = nimble_cast<MapStructTypeChanged>(mapStruct);
  EXPECT_EQ("abc", *casted.f2_ref()->f1_ref());
  EXPECT_EQ(123, *casted.f2_ref()->f2_ref());
  EXPECT_EQ(5, *casted.f5_ref());
}

TEST(NimbleForwardCompatibilityTest, MapKeyTypeChanged) {
  auto mapStruct = defaultMapStruct();
  auto casted = nimble_cast<MapStructKeyTypeChanged>(mapStruct);
  EXPECT_EQ("abc", *casted.f2_ref()->f1_ref());
  EXPECT_EQ(123, *casted.f2_ref()->f2_ref());
  EXPECT_EQ(5, *casted.f5_ref());
}

TEST(NimbleForwardCompatibilityTest, MapValueTypeChanged) {
  auto mapStruct = defaultMapStruct();
  auto casted = nimble_cast<MapStructValueTypeChanged>(mapStruct);
  EXPECT_EQ("abc", *casted.f2_ref()->f1_ref());
  EXPECT_EQ(123, *casted.f2_ref()->f2_ref());
  EXPECT_EQ(5, *casted.f5_ref());
}

} // namespace detail
} // namespace thrift
} // namespace apache
