/*
 * Copyright 2017-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * TODO(@denplusplus, by 11/04/2017) Remove.
 */
#include <string>

#include <gtest/gtest.h>

#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/protocol/TCompactProtocol.h"
#include "thrift/lib/cpp/protocol/TSimpleJSONProtocol.h"
#include "thrift/test/gen-cpp/forward_compatibility_types.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace folly;
using namespace std;

namespace {

template <typename T, typename Protocol>
shared_ptr<TMemoryBuffer> easySerialize(const T& a) {
  auto strBuffer = std::make_shared<TMemoryBuffer>();
  Protocol prot(strBuffer);
  a.write(&prot);

  return strBuffer;
}

template <typename T, typename Protocol>
T easyDeserialize(shared_ptr<TMemoryBuffer> strBuffer) {
  T u;
  Protocol prot(strBuffer);
  u.read(&prot);
  return u;
}

template <typename Protocol>
void testForwardCompatibility() {
  OldStructure s;
  s.features[1] = 1.0;
  s.features[100] = 3.14;
  static_assert(
      std::is_same<typename decltype(s.features)::key_type, int16_t>::value,
      "key must be i16");
  static_assert(
      std::is_same<typename decltype(s.features)::mapped_type, double>::value,
      "value must be double");
  auto n = easyDeserialize<NewStructure, Protocol>(
      easySerialize<OldStructure, Protocol>(s));
  EXPECT_EQ(n.features[1], 1.0);
  EXPECT_EQ(n.features[100], 3.14f);
  static_assert(
      std::is_same<typename decltype(n.features)::key_type, int32_t>::value,
      "key must be i32");
  static_assert(
      std::is_same<typename decltype(n.features)::mapped_type, float>::value,
      "value must be float");
}

template <typename Protocol>
void testForwardCompatibilityNested() {
  OldStructureNested s;
  s.featuresList.emplace_back();
  s.featuresList[0][1] = 1.0;
  s.featuresList[0][100] = 3.14;
  auto n = easyDeserialize<NewStructureNested, Protocol>(
      easySerialize<OldStructureNested, Protocol>(s));
  EXPECT_EQ(n.featuresList[0][1], 1.0);
  EXPECT_EQ(n.featuresList[0][100], 3.14f);
}

template <typename Protocol>
void testForwardCompatibilityComplexMap() {
  OldMapMapStruct s;
  s.features[1][2] = 3.14;
  s.features[2][1] = 2.71;
  auto n = easyDeserialize<NewMapMapStruct, Protocol>(
      easySerialize<OldMapMapStruct, Protocol>(s));
  EXPECT_EQ(n.features[1][2], 3.14);
  EXPECT_EQ(n.features[2][1], 2.71);
}

template <typename Protocol>
void testForwardCompatibilityComplexList() {
  OldMapListStruct s;
  s.features[1].push_back(3.14);
  s.features[1].push_back(2.71);
  s.features[2];
  s.features[3].push_back(12345.56);
  auto n = easyDeserialize<NewMapListStruct, Protocol>(
      easySerialize<OldMapListStruct, Protocol>(s));
  EXPECT_NEAR(n.features.at(1).at(0), 3.14, 1e-3);
  EXPECT_NEAR(n.features.at(1).at(1), 2.71, 1e-3);
  EXPECT_EQ(n.features.at(2).size(), 0);
  EXPECT_NEAR(n.features.at(3).at(0), 12345.56, 1e-3);
  EXPECT_EQ(n.features.size(), 3);
}

} // namespace

TEST(ForwardCompatibility1, Simple) {
  testForwardCompatibility<TBinaryProtocol>();
  testForwardCompatibility<TCompactProtocol>();
  testForwardCompatibility<TSimpleJSONProtocol>();
}

TEST(ForwardCompatibility1, Nested) {
  testForwardCompatibilityNested<TBinaryProtocol>();
  testForwardCompatibilityNested<TCompactProtocol>();
  testForwardCompatibilityNested<TSimpleJSONProtocol>();
}

TEST(ForwardCompatibility1, MapMap) {
  testForwardCompatibilityComplexMap<TBinaryProtocol>();
  testForwardCompatibilityComplexMap<TCompactProtocol>();
  testForwardCompatibilityComplexMap<TSimpleJSONProtocol>();
}

TEST(ForwardCompatibility1, MapList) {
  testForwardCompatibilityComplexList<TBinaryProtocol>();
  testForwardCompatibilityComplexList<TCompactProtocol>();
  testForwardCompatibilityComplexList<TSimpleJSONProtocol>();
}
