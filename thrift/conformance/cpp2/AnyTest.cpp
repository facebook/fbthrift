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

#include <thrift/conformance/cpp2/Any.h>

#include <folly/portability/GTest.h>
#include <thrift/conformance/cpp2/Protocol.h>
#include <thrift/conformance/cpp2/Testing.h>

namespace apache::thrift::conformance {
namespace {

TEST(AnyTest, None) {
  Any any;
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::None>());
  any.set_protocol(StandardProtocol::None);
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::None>());
  any.set_customProtocol("");
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::None>());
  any.set_customProtocol("None");
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::None>());
}

TEST(AnyTest, Standard) {
  Any any;
  any.set_protocol(StandardProtocol::Binary);
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::Binary>());
  EXPECT_TRUE(
      hasProtocol(any, getStandardProtocol<StandardProtocol::Binary>()));

  // Unnormalize name.
  any.set_protocol(StandardProtocol::None);
  any.set_customProtocol("Binary");
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::Binary>());
  EXPECT_TRUE(
      hasProtocol(any, getStandardProtocol<StandardProtocol::Binary>()));
  normalizeProtocol(any);
  EXPECT_EQ(*any.protocol_ref(), StandardProtocol::Binary);
  EXPECT_FALSE(any.customProtocol_ref().has_value());

  // Junk in the customProtocol.
  any.set_customProtocol("Ignored");
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::Binary>());
  EXPECT_TRUE(
      hasProtocol(any, getStandardProtocol<StandardProtocol::Binary>()));
  normalizeProtocol(any);
  EXPECT_EQ(*any.protocol_ref(), StandardProtocol::Binary);
  EXPECT_FALSE(any.customProtocol_ref().has_value());
}

TEST(AnyTest, Custom) {
  Any any;
  any.set_customProtocol("Hi");
  EXPECT_EQ(getProtocol(any), Protocol("Hi"));
  EXPECT_TRUE(hasProtocol(any, Protocol("Hi")));
  EXPECT_NE(getProtocol(any), Protocol("Bye"));
  EXPECT_FALSE(hasProtocol(any, Protocol("Bye")));
  EXPECT_NE(getProtocol(any), Protocol(StandardProtocol::None));
  EXPECT_FALSE(hasProtocol(any, Protocol(StandardProtocol::None)));
  EXPECT_NE(getProtocol(any), Protocol(StandardProtocol::Binary));
  EXPECT_FALSE(hasProtocol(any, Protocol(StandardProtocol::Binary)));

  // Normalization does nothing.
  normalizeProtocol(any);
  EXPECT_EQ(*any.protocol_ref(), StandardProtocol::None);
  EXPECT_EQ(any.customProtocol_ref(), "Hi");
}

TEST(AnyTest, Unknown) {
  Any any;
  any.set_protocol(kUnknownStdProtocol);
  EXPECT_EQ(getProtocol(any), UnknownProtocol());
  EXPECT_TRUE(hasProtocol(any, UnknownProtocol()));
  EXPECT_EQ(getProtocol(any).name(), "");

  // Normalization does nothing.
  normalizeProtocol(any);
  EXPECT_EQ(*any.protocol_ref(), kUnknownStdProtocol);
  EXPECT_FALSE(any.customProtocol_ref().has_value());
}

TEST(AnyTest, Custom_EmptyString) {
  Any any;
  // Empty string protocol is the same as None
  any.set_customProtocol("");
  EXPECT_TRUE(any.customProtocol_ref().has_value());
  EXPECT_EQ(getProtocol(any), kNoProtocol);
  EXPECT_TRUE(hasProtocol(any, kNoProtocol));

  // Normalization resets customProtocol.
  normalizeProtocol(any);
  EXPECT_FALSE(any.customProtocol_ref().has_value());
  EXPECT_EQ(getProtocol(any), kNoProtocol);
  EXPECT_TRUE(hasProtocol(any, kNoProtocol));
}

} // namespace
} // namespace apache::thrift::conformance
