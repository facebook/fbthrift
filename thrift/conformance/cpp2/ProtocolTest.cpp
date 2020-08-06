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

#include "thrift/conformance/cpp2/Protocol.h"

#include <gtest/gtest.h>

namespace apache::thrift::conformance {

namespace {

template <StandardProtocol StdProtocol>
void testStandardProtocol(std::string_view expectedName) {
  const auto& protocol = createProtocol(StdProtocol);
  EXPECT_EQ(createProtocol(StdProtocol), protocol);
  EXPECT_EQ(createProtocol(std::string(expectedName)), protocol);
  EXPECT_EQ(getProtocolName(protocol), expectedName);
  auto normalized = protocol;
  normalizeProtocol(&normalized);
  EXPECT_EQ(normalized, protocol);

  normalized.set_custom(std::string(expectedName));
  EXPECT_NE(normalized, protocol);
  normalizeProtocol(&normalized);
  EXPECT_EQ(normalized, protocol);
}

TEST(ProtocolTest, Standard) {
  testStandardProtocol<StandardProtocol::Binary>("Binary");
  testStandardProtocol<StandardProtocol::Compact>("Compact");
  testStandardProtocol<StandardProtocol::Json>("Json");
  testStandardProtocol<StandardProtocol::SimpleJson>("SimpleJson");
}

TEST(ProtocolTest, Custom) {
  auto protocol = createProtocol("hi");
  EXPECT_EQ(createProtocol("hi"), protocol);
  EXPECT_NE(createProtocol("bye"), protocol);
  EXPECT_EQ(getProtocolName(protocol), "hi");
  normalizeProtocol(&protocol);
  EXPECT_EQ(protocol, createProtocol("hi"));
}

TEST(ProtocolIdManagerTest, Standard) {
  ProtocolIdManager ids;

  // All ways of specifying a standard protocol return the correct id.
  EXPECT_EQ(
      ids.getId(createProtocol(StandardProtocol::Binary)),
      static_cast<ProtocolIdManager::id_type>(StandardProtocol::Binary));
  Protocol protocol;
  protocol.set_custom("Binary");
  EXPECT_EQ(
      ids.getId(protocol),
      static_cast<ProtocolIdManager::id_type>(StandardProtocol::Binary));
  EXPECT_EQ(
      ids.getOrCreateId(protocol),
      static_cast<ProtocolIdManager::id_type>(StandardProtocol::Binary));
}

TEST(ProtocolIdManagerTest, Custom) {
  ProtocolIdManager ids;
  EXPECT_EQ(ids.getId(createProtocol(static_cast<StandardProtocol>(100))), 100);
  EXPECT_EQ(ids.getId(createProtocol("hi")), ProtocolIdManager::kNoId);

  EXPECT_EQ(ids.getOrCreateId(createProtocol("hi")), -1);
  EXPECT_EQ(ids.getOrCreateId(createProtocol("bye")), -2);
  EXPECT_EQ(ids.getId(createProtocol("hi")), -1);
  EXPECT_EQ(ids.getOrCreateId(createProtocol("hi")), -1);
}

TEST(ProtocolIdManagerTest, Empty) {
  ProtocolIdManager ids;

  EXPECT_EQ(ids.getId({}), ProtocolIdManager::kNoId);
  EXPECT_THROW(ids.getOrCreateId({}), std::invalid_argument);
}

} // namespace
} // namespace apache::thrift::conformance
