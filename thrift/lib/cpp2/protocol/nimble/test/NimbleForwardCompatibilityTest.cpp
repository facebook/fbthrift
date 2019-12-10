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
  result.f1 = 1;
  result.f2 = 2;
  result.f3 = 3;
  result.f4 = 4;
  result.f5 = "5";
  result.f6 = "6";
  result.f7 = "7";
  result.f8 = 8.0f;
  result.f9 = 9.0;
  return result;
}

BigFieldIds defaultBigFieldIds() {
  BigFieldIds result;
  result.f1 = 1;
  result.f100 = 100;
  result.f2 = 2;
  result.f101 = 101;
  result.f102 = 102;
  result.f1000 = 1000;
  result.f1001 = 1001;
  result.f3 = 3;
  result.f4 = 4;
  return result;
}

NestedStructL2 defaultNestedStructL2() {
  NestedStructL2 result;
  result.f1 = "1";
  result.f2.f1 = 1;
  result.f2.f2 = 2;
  result.f2.f3.f1 = 1;
  result.f2.f3.f2 = 2;
  result.f3 = 3;
  return result;
}

ListStructElem defaultListStructElem() {
  ListStructElem result;
  result.f1 = "1";
  result.f2 = 2;
  return result;
}

ListStruct defaultListStruct() {
  ListStruct result;
  result.f1 = std::vector<ListStructElem>(10, defaultListStructElem());
  result.f2.f1 = "unusual";
  result.f2.f2 = 123;
  result.f3 = 456;
  result.f4 = {1, 2, 3, 4, 5};
  result.f5 = 789;
  return result;
}

MapStruct defaultMapStruct() {
  MapStructValue mapValue;
  mapValue.f1 = "1";
  mapValue.f2 = 2;

  MapStruct result;
  result.f1 = {{1, mapValue}, {2, mapValue}, {3, mapValue}};
  result.f2.f1 = "abc";
  result.f2.f2 = 123;
  result.f5 = 5;
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
  EXPECT_EQ(1, casted.f1);
  EXPECT_EQ(2, casted.f2);
  // Altered
  EXPECT_EQ(0, casted.f3);
  EXPECT_EQ(4, casted.f4);
  // Altered
  EXPECT_EQ("", casted.f5);
  EXPECT_EQ("6", casted.f6);
}

TEST(NimbleForwardCompatibilityTest, PrimitiveConsecutiveMissing) {
  auto primitives = defaultPrimitives();
  auto casted = nimble_cast<PrimitivesConsecutiveMissing>(primitives);

  EXPECT_EQ(1, casted.f1);
  EXPECT_EQ("6", casted.f6);
}

TEST(NimbleForwardCompatibilityTest, PrimitivesTypesChanged) {
  auto primitives = defaultPrimitives();
  auto casted = nimble_cast<PrimitivesTypesChanged>(primitives);

  // Altered
  EXPECT_EQ(0, casted.f1);
  EXPECT_EQ(2, casted.f2);
  // Altered
  EXPECT_EQ(0, casted.f3);
  EXPECT_EQ(4, casted.f4);
  // Altered
  EXPECT_EQ(0.0, casted.f5);
  // Altered
  EXPECT_TRUE(casted.f6.empty());
  EXPECT_EQ("7", casted.f7);
  EXPECT_EQ(8.0f, casted.f8);
  // Altered
  EXPECT_EQ(0.0, casted.f9);
}

TEST(NimbleForwardCompatibilityTest, PrimitivesTypesReordered) {
  auto primitives = defaultPrimitives();
  auto casted = nimble_cast<PrimitivesTypesReordered>(primitives);

  EXPECT_EQ(primitives.f1, casted.f1);
  EXPECT_EQ(primitives.f2, casted.f2);
  EXPECT_EQ(primitives.f3, casted.f3);
  EXPECT_EQ(primitives.f4, casted.f4);
  EXPECT_EQ(primitives.f5, casted.f5);
  EXPECT_EQ(primitives.f6, casted.f6);
  EXPECT_EQ(primitives.f7, casted.f7);
  EXPECT_EQ(primitives.f8, casted.f8);
  EXPECT_EQ(primitives.f9, casted.f9);
}

TEST(NimbleForwardCompatibilityTest, BigFieldIds) {
  BigFieldIds bigFieldIds;
  bigFieldIds.f1 = 1;
  bigFieldIds.f100 = 100;
  bigFieldIds.f2 = 2;
  bigFieldIds.f101 = 101;
  bigFieldIds.f102 = 102;
  bigFieldIds.f1000 = 1000;
  bigFieldIds.f1001 = 1001;
  bigFieldIds.f3 = 3;
  bigFieldIds.f4 = 4;

  auto bigFieldIdsMissing = nimble_cast<BigFieldIdsMissing>(bigFieldIds);
  EXPECT_EQ(1, bigFieldIdsMissing.f1);
  EXPECT_EQ(2, bigFieldIdsMissing.f2);
  EXPECT_EQ(4, bigFieldIdsMissing.f4);
}

TEST(NimbleForwardCompatibilityTest, NestedStruct) {
  NestedStructL2 nested;
  nested.f1 = "1";
  nested.f2.f1 = 1;
  nested.f2.f2 = 2;
  nested.f2.f3.f1 = 1;
  nested.f2.f3.f2 = 2;
  nested.f3 = 3;

  auto casted = nimble_cast<NestedStructMissingSubstruct>(nested);
  EXPECT_EQ("1", casted.f1);
  EXPECT_EQ(3, casted.f3);
}

TEST(NimbleForwardCompatibilityTest, NestedStructTypeChanged) {
  auto nested = defaultNestedStructL2();
  auto casted = nimble_cast<NestedStructTypeChanged>(nested);
  EXPECT_EQ("1", casted.f1);
  EXPECT_EQ(3, casted.f3);
}

TEST(NimbleForwardCompatabilityTest, Lists) {
  auto listStruct = defaultListStruct();
  auto casted = nimble_cast<ListStructMissingFields>(listStruct);
  EXPECT_EQ("unusual", casted.f2.f1);
  EXPECT_EQ(123, casted.f2.f2);
  EXPECT_EQ(456, casted.f3);
  EXPECT_EQ(789, casted.f5);
}

TEST(NimbleForwardCompatibilityTest, ListTypeChanged) {
  auto listStruct = defaultListStruct();
  auto casted = nimble_cast<ListStructTypeChanged>(listStruct);
  EXPECT_EQ("unusual", casted.f2.f1);
  EXPECT_EQ(123, casted.f2.f2);
  EXPECT_EQ(456, casted.f3);
  std::vector<int32_t> expectedList = {1, 2, 3, 4, 5};
  EXPECT_EQ(expectedList, casted.f4);
  EXPECT_EQ(789, casted.f5);
}

TEST(NimbleForwardCompatibilityTest, ListElemTypeChanged) {
  auto listStruct = defaultListStruct();
  auto casted = nimble_cast<ListStructListElemTypeChanged>(listStruct);
  EXPECT_EQ("unusual", casted.f2.f1);
  EXPECT_EQ(123, casted.f2.f2);
  EXPECT_EQ(456, casted.f3);
  std::vector<int32_t> expectedList = {1, 2, 3, 4, 5};
  EXPECT_EQ(expectedList, casted.f4);
  EXPECT_EQ(789, casted.f5);
}

TEST(NimbleForwardCompatibilityTest, Maps) {
  auto mapStruct = defaultMapStruct();
  auto casted = nimble_cast<MapStructMissingFields>(mapStruct);
  EXPECT_EQ("abc", casted.f2.f1);
  EXPECT_EQ(123, casted.f2.f2);
}

TEST(NimbleForwardCompatibilityTest, MapTypeChanged) {
  auto mapStruct = defaultMapStruct();
  auto casted = nimble_cast<MapStructTypeChanged>(mapStruct);
  EXPECT_EQ("abc", casted.f2.f1);
  EXPECT_EQ(123, casted.f2.f2);
  EXPECT_EQ(5, casted.f5);
}

TEST(NimbleForwardCompatibilityTest, MapKeyTypeChanged) {
  auto mapStruct = defaultMapStruct();
  auto casted = nimble_cast<MapStructKeyTypeChanged>(mapStruct);
  EXPECT_EQ("abc", casted.f2.f1);
  EXPECT_EQ(123, casted.f2.f2);
  EXPECT_EQ(5, casted.f5);
}

TEST(NimbleForwardCompatibilityTest, MapValueTypeChanged) {
  auto mapStruct = defaultMapStruct();
  auto casted = nimble_cast<MapStructValueTypeChanged>(mapStruct);
  EXPECT_EQ("abc", casted.f2.f1);
  EXPECT_EQ(123, casted.f2.f2);
  EXPECT_EQ(5, casted.f5);
}

} // namespace detail
} // namespace thrift
} // namespace apache
