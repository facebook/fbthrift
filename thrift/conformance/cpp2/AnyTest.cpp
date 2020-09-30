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
  validateAny(any);
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::None>());
  any.set_protocol(StandardProtocol::None);
  validateAny(any);
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::None>());
  any.set_customProtocol("");
  validateAny(any);
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::None>());

  any.set_customProtocol("None");
  EXPECT_NE(getProtocol(any), getStandardProtocol<StandardProtocol::None>());
  EXPECT_THROW(validateAny(any), std::invalid_argument);
}

TEST(AnyTest, Standard) {
  Any any;
  any.set_protocol(StandardProtocol::Binary);
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::Binary>());
  EXPECT_TRUE(
      hasProtocol(any, getStandardProtocol<StandardProtocol::Binary>()));

  // Junk in the customProtocol.
  any.set_customProtocol("Ignored");
  EXPECT_EQ(getProtocol(any), getStandardProtocol<StandardProtocol::Binary>());
  EXPECT_TRUE(
      hasProtocol(any, getStandardProtocol<StandardProtocol::Binary>()));

  // Unnormalize name.
  any.set_protocol(StandardProtocol::None);
  any.set_customProtocol("Binary");
  EXPECT_NE(getProtocol(any), getStandardProtocol<StandardProtocol::Binary>());
  EXPECT_THROW(validateAny(any), std::invalid_argument);
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
}

TEST(AnyTest, Unknown) {
  Any any;
  any.set_protocol(kUnknownStdProtocol);
  EXPECT_EQ(getProtocol(any), UnknownProtocol());
  EXPECT_TRUE(hasProtocol(any, UnknownProtocol()));
  EXPECT_EQ(getProtocol(any).name(), "");
}

TEST(AnyTest, Custom_EmptyString) {
  Any any;
  // Empty string protocol is the same as None
  any.set_customProtocol("");
  EXPECT_TRUE(any.customProtocol_ref().has_value());
  EXPECT_EQ(getProtocol(any), kNoProtocol);
  EXPECT_TRUE(hasProtocol(any, kNoProtocol));
}

TEST(AnyTest, ValidateAny) {
  const auto bad = "foo.com:42/my/type";
  const auto good = "foo.com/my/type";
  Any any;
  validateAny(any);
  any.set_type("");
  any.set_customProtocol("");
  validateAny(any);
  any.type_ref().ensure() = bad;
  EXPECT_THROW(validateAny(any), std::invalid_argument);
  any.type_ref() = good;
  any.customProtocol_ref().ensure() = bad;
  EXPECT_THROW(validateAny(any), std::invalid_argument);
  any.customProtocol_ref() = good;
  validateAny(any);
}

TEST(AnyTest, ValidateAnyType) {
  const auto bad = "foo.com:42/my/type";
  const auto good = "foo.com/my/type";
  AnyType type;
  EXPECT_THROW(validateAnyType(type), std::invalid_argument);
  type.name_ref() = good;
  validateAnyType(type);
  type.aliases_ref()->emplace_back(good);
  validateAnyType(type);
  type.set_typeIdBytes(any_constants::minTypeIdBytes());
  validateAnyType(type);
  type.set_typeIdBytes(any_constants::maxTypeIdBytes());
  validateAnyType(type);

  {
    AnyType badType(type);
    badType.set_name(bad);
    EXPECT_THROW(validateAnyType(badType), std::invalid_argument);
  }

  {
    AnyType badType(type);
    badType.aliases_ref()->emplace_back(bad);
    EXPECT_THROW(validateAnyType(badType), std::invalid_argument);
  }

  {
    AnyType badType(type);
    badType.set_typeIdBytes(1);
    EXPECT_THROW(validateAnyType(badType), std::invalid_argument);
  }
  {
    AnyType badType(type);
    badType.set_typeIdBytes(100);
    EXPECT_THROW(validateAnyType(badType), std::invalid_argument);
  }
}

} // namespace
} // namespace apache::thrift::conformance
