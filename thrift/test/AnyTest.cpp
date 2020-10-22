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
#include <thrift/conformance/cpp2/AnyRegistry.h>

namespace apache::thrift::test {
namespace {

template <conformance::StandardProtocol P>
bool isRegistered(std::string_view name) {
  return conformance::AnyRegistry::generated().getSerializerByName(
             name, conformance::getStandardProtocol<P>()) != nullptr;
}

TEST(AnyTest, Registered) {
  constexpr auto kType1Name = "facebook.com/thrift/test/AnyTest1Struct";
  EXPECT_TRUE(isRegistered<conformance::StandardProtocol::Binary>(kType1Name));
  EXPECT_TRUE(isRegistered<conformance::StandardProtocol::Compact>(kType1Name));
  EXPECT_FALSE(
      isRegistered<conformance::StandardProtocol::SimpleJson>(kType1Name));
  EXPECT_FALSE(isRegistered<conformance::StandardProtocol::Json>(kType1Name));

  // Has Json enabled.
  constexpr auto kType2Name = "facebook.com/thrift/test/AnyTest2Struct";
  EXPECT_TRUE(isRegistered<conformance::StandardProtocol::Binary>(kType2Name));
  EXPECT_TRUE(isRegistered<conformance::StandardProtocol::Compact>(kType2Name));
  EXPECT_TRUE(
      isRegistered<conformance::StandardProtocol::SimpleJson>(kType2Name));
  EXPECT_FALSE(isRegistered<conformance::StandardProtocol::Json>(kType2Name));

  // Does not have any enabled.
  constexpr auto kType3Name = "facebook.com/thrift/test/AnyTest3Struct";
  EXPECT_FALSE(isRegistered<conformance::StandardProtocol::Binary>(kType3Name));
  EXPECT_FALSE(
      isRegistered<conformance::StandardProtocol::Compact>(kType3Name));
  EXPECT_FALSE(
      isRegistered<conformance::StandardProtocol::SimpleJson>(kType3Name));
  EXPECT_FALSE(isRegistered<conformance::StandardProtocol::Json>(kType3Name));
}

} // namespace
} // namespace apache::thrift::test
