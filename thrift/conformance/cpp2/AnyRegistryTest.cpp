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

#include <thrift/conformance/cpp2/AnyRegistry.h>

#include <gtest/gtest.h>

#include <thrift/conformance/cpp2/Testing.h>

namespace apache::thrift::conformance {

namespace {

std::string toString(const folly::IOBuf& buf) {
  std::string result;
  folly::IOBufQueue queue;
  queue.append(buf);
  queue.appendToString(result);
  return result;
}

TEST(AnyTest, Registry) {
  AnyRegistry registry;
  EXPECT_EQ(registry.getTypeName(typeid(int)), "");
  EXPECT_EQ(registry.getSerializer("int", kFollyToStringProtocol), nullptr);
  EXPECT_EQ(
      registry.getSerializer(typeid(int), kFollyToStringProtocol), nullptr);

  FollyToStringSerializer<int> intCodec;

  // Type must be registered first.
  EXPECT_THROW(registry.registerSerializer<int>(&intCodec), std::out_of_range);

  // Empty string is rejected.
  EXPECT_FALSE(registry.registerType<int>(""));

  EXPECT_TRUE(registry.registerType<int>("int"));
  EXPECT_EQ(registry.getTypeName(typeid(int)), "int");
  EXPECT_EQ(registry.getSerializer("int", kFollyToStringProtocol), nullptr);
  EXPECT_EQ(
      registry.getSerializer(typeid(int), kFollyToStringProtocol), nullptr);

  // Conflicting and duplicate registrations are rejected.
  EXPECT_FALSE(registry.registerType<int>("int"));
  EXPECT_FALSE(registry.registerType<int>("other int"));
  EXPECT_FALSE(registry.registerType<double>("int"));

  EXPECT_TRUE(registry.registerSerializer<int>(&intCodec));
  EXPECT_EQ(registry.getTypeName(typeid(int)), "int");
  EXPECT_EQ(registry.getSerializer("int", kFollyToStringProtocol), &intCodec);
  EXPECT_EQ(
      registry.getSerializer(typeid(int), kFollyToStringProtocol), &intCodec);

  // Duplicate registrations are rejected.
  EXPECT_FALSE(registry.registerSerializer<int>(&intCodec));

  Number1Serializer number1Codec;
  EXPECT_TRUE(registry.registerSerializer<int>(&number1Codec));

  EXPECT_TRUE(registry.registerType<double>("double"));

  // nullptr is rejected.
  EXPECT_FALSE(registry.registerSerializer<double>(nullptr));

  EXPECT_TRUE(registry.registerSerializer<double>(
      std::make_unique<FollyToStringSerializer<double>>()));

  Any value = registry.store(3, kFollyToStringProtocol);
  EXPECT_EQ(*value.type_ref(), "int");
  EXPECT_EQ(toString(*value.data_ref()), "3");
  ASSERT_EQ(value.protocol_ref()->getType(), Protocol::custom);
  EXPECT_EQ(value.protocol_ref()->get_custom(), "FollyToString");
  EXPECT_EQ(std::any_cast<int>(registry.load(value)), 3);
  EXPECT_EQ(registry.load<int>(value), 3);

  // Storing an Any does nothing if the protocols match.
  Any original = value;
  value = registry.store(original, kFollyToStringProtocol);
  EXPECT_EQ(*value.type_ref(), "int");
  EXPECT_EQ(toString(*value.data_ref()), "3");
  ASSERT_EQ(value.protocol_ref()->getType(), Protocol::custom);
  EXPECT_EQ(value.protocol_ref()->get_custom(), "FollyToString");
  value = registry.store(std::any(std::move(original)), kFollyToStringProtocol);
  EXPECT_EQ(*value.type_ref(), "int");
  EXPECT_EQ(toString(*value.data_ref()), "3");
  ASSERT_EQ(value.protocol_ref()->getType(), Protocol::custom);
  EXPECT_EQ(value.protocol_ref()->get_custom(), "FollyToString");

  // Storing an Any with a different protocol does a conversion.
  original = value;
  value = registry.store(original, Number1Serializer::kProtocol);
  EXPECT_EQ(*value.type_ref(), "int");
  EXPECT_EQ(toString(*value.data_ref()), "number 1!!");
  ASSERT_EQ(value.protocol_ref()->getType(), Protocol::custom);
  EXPECT_EQ(value.protocol_ref()->get_custom(), "Number1");
  value = registry.store(
      std::any(std::move(original)), Number1Serializer::kProtocol);
  EXPECT_EQ(*value.type_ref(), "int");
  EXPECT_EQ(toString(*value.data_ref()), "number 1!!");
  ASSERT_EQ(value.protocol_ref()->getType(), Protocol::custom);
  EXPECT_EQ(value.protocol_ref()->get_custom(), "Number1");
  EXPECT_EQ(std::any_cast<int>(registry.load(value)), 1);
  EXPECT_EQ(registry.load<int>(value), 1);

  // Storing an unsupported type is an error.
  EXPECT_THROW(registry.store(2.5f, kFollyToStringProtocol), std::bad_any_cast);
  EXPECT_THROW(
      registry.store(std::any(2.5f), kFollyToStringProtocol),
      std::bad_any_cast);

  // Storing using an unsupported protocol throws an error
  EXPECT_THROW(
      registry.store(3, createProtocol(StandardProtocol::Binary)),
      std::bad_any_cast);

  // Loading an empty Any value throws an error.
  value = {};
  EXPECT_EQ(*value.type_ref(), "");
  EXPECT_EQ(toString(*value.data_ref()), "");
  ASSERT_EQ(value.protocol_ref()->getType(), Protocol::__EMPTY__);
  EXPECT_THROW(registry.load(value), std::bad_any_cast);
  EXPECT_THROW(registry.load<float>(value), std::bad_any_cast);

  value = registry.store(2.5, kFollyToStringProtocol);
  EXPECT_EQ(*value.type_ref(), "double");
  EXPECT_EQ(toString(*value.data_ref()), "2.5");
  ASSERT_EQ(value.protocol_ref()->getType(), Protocol::custom);
  EXPECT_EQ(value.protocol_ref()->get_custom(), "FollyToString");
  EXPECT_EQ(std::any_cast<double>(registry.load(value)), 2.5);
  EXPECT_EQ(registry.load<double>(value), 2.5);
  EXPECT_THROW(registry.load<int>(value), std::bad_any_cast);
}

} // namespace
} // namespace apache::thrift::conformance
