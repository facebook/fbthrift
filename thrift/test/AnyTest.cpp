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

using conformance::StandardProtocol;
template <conformance::StandardProtocol P>
bool isRegistered(std::string_view name) {
  return conformance::AnyRegistry::generated().getSerializerByName(
             name, conformance::getStandardProtocol<P>()) != nullptr;
}

TEST(AnyTest, Registered) {
  {
    constexpr auto kTypeName = "facebook.com/thrift/test/AnyTestStruct";
    EXPECT_TRUE(isRegistered<StandardProtocol::Binary>(kTypeName));
    EXPECT_TRUE(isRegistered<StandardProtocol::Compact>(kTypeName));
    EXPECT_FALSE(isRegistered<StandardProtocol::SimpleJson>(kTypeName));
    EXPECT_FALSE(isRegistered<StandardProtocol::Json>(kTypeName));
  }

  {
    constexpr auto kTypeName = "facebook.com/thrift/test/AnyTestException";
    EXPECT_TRUE(isRegistered<StandardProtocol::Binary>(kTypeName));
    EXPECT_TRUE(isRegistered<StandardProtocol::Compact>(kTypeName));
    EXPECT_FALSE(isRegistered<StandardProtocol::SimpleJson>(kTypeName));
    EXPECT_FALSE(isRegistered<StandardProtocol::Json>(kTypeName));
  }

  { // Has Json enabled.
    constexpr auto kTypeName = "facebook.com/thrift/test/AnyTestUnion";
    EXPECT_TRUE(isRegistered<StandardProtocol::Binary>(kTypeName));
    EXPECT_TRUE(isRegistered<StandardProtocol::Compact>(kTypeName));
    EXPECT_TRUE(isRegistered<StandardProtocol::SimpleJson>(kTypeName));
    EXPECT_FALSE(isRegistered<StandardProtocol::Json>(kTypeName));
  }

  { // Does not have `any` in buck target options.
    constexpr auto kTypeName =
        "facebook.com/thrift/test/AnyTestMissingAnyOption";
    EXPECT_FALSE(isRegistered<StandardProtocol::Binary>(kTypeName));
    EXPECT_FALSE(isRegistered<StandardProtocol::Compact>(kTypeName));
    EXPECT_FALSE(isRegistered<StandardProtocol::SimpleJson>(kTypeName));
    EXPECT_FALSE(isRegistered<StandardProtocol::Json>(kTypeName));
  }
}

} // namespace
} // namespace apache::thrift::test
