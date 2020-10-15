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

#include <folly/portability/GTest.h>

#include <thrift/conformance/cpp2/Any.h>
#include <thrift/conformance/cpp2/Object.h>
#include <thrift/conformance/cpp2/Testing.h>
#include <thrift/conformance/cpp2/UniversalType.h>

namespace apache::thrift::conformance {

namespace {

TEST(AnyRegistryTest, ShortType) {
  AnyRegistry registry;
  FollyToStringSerializer<int> intCodec;
  EXPECT_TRUE(registry.registerType<int>(shortAnyType(), {&intCodec}));

  // Should use the type name because it is shorter than the id.
  Any any = registry.store(1, kFollyToStringProtocol);
  EXPECT_TRUE(any.type_ref());
  EXPECT_FALSE(any.typeId_ref());
  EXPECT_EQ(registry.load<int>(any), 1);
}

constexpr int kUnsetTypeId = -1;
void checkLongType(int typeBytes, int expectedOutBytes) {
  AnyRegistry registry;
  FollyToStringSerializer<int> intCodec;
  auto longType = longAnyType();
  if (typeBytes == kUnsetTypeId) {
    longType.typeIdBytes_ref().reset();
  } else {
    longType.set_typeIdBytes(typeBytes);
  }
  EXPECT_TRUE(registry.registerType<int>(longType, {&intCodec}));

  // Should use the type id because it is shorter than the name.
  Any any = registry.store(1, kFollyToStringProtocol);
  if (expectedOutBytes == kDisableTypeId) {
    EXPECT_FALSE(any.typeId_ref());
    EXPECT_TRUE(any.type_ref());
    EXPECT_EQ(
        registry.getSerializerByName(*any.get_type(), intCodec.getProtocol()),
        &intCodec);
  } else {
    EXPECT_FALSE(any.type_ref());
    ASSERT_TRUE(any.typeId_ref());
    EXPECT_EQ(any.typeId_ref().value_unchecked().size(), expectedOutBytes);
    EXPECT_EQ(
        registry.getSerializerById(*any.get_typeId(), intCodec.getProtocol()),
        &intCodec);
  }
  EXPECT_EQ(registry.load<int>(any), 1);
}

TEST(AnyRegistryTest, LongType) {
  // Disabled is respected.
  THRIFT_SCOPED_CHECK(checkLongType(kDisableTypeId, kDisableTypeId));

  // Unset uses minimum.
  THRIFT_SCOPED_CHECK(checkLongType(kUnsetTypeId, kMinTypeIdBytes));

  // Type can increase bytes used.
  THRIFT_SCOPED_CHECK(checkLongType(24, 24));
  THRIFT_SCOPED_CHECK(checkLongType(32, 32));
}

TEST(AnyRegistryTest, ShortTypeId) {
  AnyRegistry registry;
  FollyToStringSerializer<int> intCodec;
  auto longType = longAnyType();
  EXPECT_TRUE(registry.registerType<int>(longType, {&intCodec}));
  Any any = registry.store(1, kFollyToStringProtocol);
  any.set_typeId(any.typeId_ref()->substr(0, 8));
  EXPECT_THROW(registry.load<int>(any), std::out_of_range);
}

TEST(AnyRegistryTest, TypeNotFound) {
  AnyRegistry registry;
  EXPECT_EQ(registry.getTypeName<int>(), "");
  EXPECT_EQ((registry.getSerializer<int, StandardProtocol::Binary>()), nullptr);

  EXPECT_THROW(registry.store<StandardProtocol::Binary>(1), std::out_of_range);

  Any any;
  EXPECT_THROW(registry.load<int>(any), std::out_of_range);
  any.set_type(thriftType("int"));
  EXPECT_THROW(registry.load<int>(any), std::out_of_range);
  any.set_protocol(StandardProtocol::Binary);
  EXPECT_THROW(registry.load<int>(any), std::out_of_range);

  FollyToStringSerializer<int> intCodec;
  EXPECT_THROW(registry.registerSerializer<int>(&intCodec), std::out_of_range);
}

TEST(AnyRegistryTest, ProtocolNotFound) {
  AnyRegistry registry;
  EXPECT_TRUE(registry.registerType<int>(testAnyType("int")));
  EXPECT_EQ(registry.getTypeName<int>(), thriftType("int"));
  EXPECT_EQ((registry.getSerializer<int, StandardProtocol::Binary>()), nullptr);

  EXPECT_THROW(registry.store<StandardProtocol::Binary>(1), std::out_of_range);

  Any any;
  EXPECT_THROW(registry.load<int>(any), std::out_of_range);
  any.set_type(thriftType("int"));
  EXPECT_THROW(registry.load<int>(any), std::out_of_range);
  any.set_protocol(StandardProtocol::Binary);
  EXPECT_THROW(registry.load<int>(any), std::out_of_range);
}

TEST(AnyRegistryTest, TypeIdToShort) {
  AnyRegistry registry;
  FollyToStringSerializer<int> intCodec;
  auto anyType = longAnyType();
  anyType.set_typeIdBytes(17);
  EXPECT_TRUE(registry.registerType<int>(anyType, {&intCodec}));
  Any any = registry.store(1, intCodec.getProtocol());
  ASSERT_TRUE(any.typeId_ref());
  EXPECT_EQ(any.get_typeId()->size(), 17);
  EXPECT_EQ(registry.load<int>(any), 1);

  any.set_typeId(any.get_typeId()->substr(0, 16));
  EXPECT_EQ(registry.load<int>(any), 1);

  any.set_typeId(any.get_typeId()->substr(0, 15));
  EXPECT_THROW(registry.load<int>(any), std::out_of_range);
}

TEST(AnyRegistryTest, Behavior) {
  AnyRegistry registry;
  const AnyRegistry& cregistry = registry;
  EXPECT_EQ(cregistry.getTypeName(typeid(int)), "");
  EXPECT_EQ(cregistry.getSerializer<int>(kFollyToStringProtocol), nullptr);

  FollyToStringSerializer<int> intCodec;

  // Type must be registered first.
  EXPECT_THROW(registry.registerSerializer<int>(&intCodec), std::out_of_range);

  // Empty string is rejected.
  EXPECT_THROW(
      registry.registerType<int>(testAnyType("")), std::invalid_argument);

  EXPECT_TRUE(registry.registerType<int>(testAnyType("int")));
  EXPECT_EQ(cregistry.getTypeName(typeid(int)), thriftType("int"));
  EXPECT_EQ(cregistry.getSerializer<int>(kFollyToStringProtocol), nullptr);

  // Conflicting and duplicate registrations are rejected.
  EXPECT_FALSE(registry.registerType<int>(testAnyType("int")));
  EXPECT_FALSE(registry.registerType<int>(testAnyType("other-int")));
  EXPECT_FALSE(registry.registerType<double>(testAnyType("int")));

  EXPECT_TRUE(registry.registerSerializer<int>(&intCodec));
  EXPECT_EQ(cregistry.getTypeName<int>(), thriftType("int"));
  EXPECT_EQ(cregistry.getSerializer<int>(kFollyToStringProtocol), &intCodec);

  // Duplicate registrations are rejected.
  EXPECT_FALSE(registry.registerSerializer<int>(&intCodec));

  Number1Serializer number1Codec;
  EXPECT_TRUE(registry.registerSerializer<int>(&number1Codec));

  EXPECT_TRUE(registry.registerType<double>(testAnyType("double")));

  // nullptr is rejected.
  EXPECT_FALSE(registry.registerSerializer<double>(nullptr));

  EXPECT_TRUE(registry.registerSerializer<double>(
      std::make_unique<FollyToStringSerializer<double>>()));

  Any value = cregistry.store(3, kFollyToStringProtocol);
  EXPECT_EQ(value.type_ref().value_or(""), thriftType("int"));
  EXPECT_FALSE(value.typeId_ref().has_value());
  EXPECT_EQ(toString(*value.data_ref()), "3");
  EXPECT_TRUE(hasProtocol(value, kFollyToStringProtocol));
  EXPECT_EQ(std::any_cast<int>(cregistry.load(value)), 3);
  EXPECT_EQ(cregistry.load<int>(value), 3);

  // Storing an Any does nothing if the protocols match.
  Any original = value;
  value = cregistry.store(original, kFollyToStringProtocol);
  EXPECT_EQ(value.type_ref().value_or(""), thriftType("int"));
  EXPECT_FALSE(value.typeId_ref().has_value());
  EXPECT_EQ(toString(*value.data_ref()), "3");
  EXPECT_TRUE(hasProtocol(value, kFollyToStringProtocol));
  value =
      cregistry.store(std::any(std::move(original)), kFollyToStringProtocol);
  EXPECT_EQ(value.type_ref().value_or(""), thriftType("int"));
  EXPECT_FALSE(value.typeId_ref().has_value());
  EXPECT_EQ(toString(*value.data_ref()), "3");
  EXPECT_TRUE(hasProtocol(value, kFollyToStringProtocol));

  // Storing an Any with a different protocol does a conversion.
  original = value;
  value = cregistry.store(original, Number1Serializer::kProtocol);
  EXPECT_EQ(value.type_ref().value_or(""), thriftType("int"));
  EXPECT_FALSE(value.typeId_ref().has_value());
  EXPECT_EQ(toString(*value.data_ref()), "number 1!!");
  EXPECT_TRUE(hasProtocol(value, Number1Serializer::kProtocol));
  value = cregistry.store(
      std::any(std::move(original)), Number1Serializer::kProtocol);
  EXPECT_EQ(value.type_ref().value_or(""), thriftType("int"));
  EXPECT_FALSE(value.typeId_ref().has_value());
  EXPECT_EQ(toString(*value.data_ref()), "number 1!!");
  EXPECT_TRUE(hasProtocol(value, Number1Serializer::kProtocol));
  EXPECT_EQ(std::any_cast<int>(cregistry.load(value)), 1);
  EXPECT_EQ(cregistry.load<int>(value), 1);

  // Storing an unsupported type is an error.
  EXPECT_THROW(
      cregistry.store(2.5f, kFollyToStringProtocol), std::out_of_range);
  EXPECT_THROW(
      cregistry.store(std::any(2.5f), kFollyToStringProtocol),
      std::out_of_range);

  // Storing using an unsupported protocol throws an error
  EXPECT_THROW(
      cregistry.store(3, Protocol(StandardProtocol::Binary)),
      std::out_of_range);

  // Loading an empty Any value throws an error.
  value = {};
  EXPECT_FALSE(value.type_ref().has_value());
  EXPECT_FALSE(value.typeId_ref().has_value());
  EXPECT_EQ(toString(*value.data_ref()), "");
  EXPECT_TRUE(hasProtocol(value, Protocol{}));
  EXPECT_THROW(cregistry.load(value), std::out_of_range);
  EXPECT_THROW(cregistry.load<float>(value), std::out_of_range);
  value.set_type("foo");
  EXPECT_THROW(cregistry.load(value), std::out_of_range);
  EXPECT_THROW(cregistry.load<float>(value), std::out_of_range);

  value = cregistry.store(2.5, kFollyToStringProtocol);
  EXPECT_EQ(*value.type_ref(), thriftType("double"));
  EXPECT_FALSE(value.typeId_ref().has_value());
  EXPECT_EQ(toString(*value.data_ref()), "2.5");
  EXPECT_TRUE(hasProtocol(value, kFollyToStringProtocol));
  EXPECT_EQ(std::any_cast<double>(cregistry.load(value)), 2.5);
  EXPECT_EQ(cregistry.load<double>(value), 2.5);
  EXPECT_THROW(cregistry.load<int>(value), std::bad_any_cast);
}

TEST(AnyRegistryTest, Aliases) {
  AnyRegistry registry;
  const AnyRegistry& cregistry = registry;
  FollyToStringSerializer<int> intCodec;
  Number1Serializer oneCodec;

  EXPECT_TRUE(registry.registerType<int>(
      testAnyType({"int", "Int", "Integer"}), {&oneCodec, &intCodec}));
  EXPECT_EQ(registry.getTypeName<int>(), thriftType("int"));
  EXPECT_EQ(
      registry.getSerializerByName(thriftType("int"), oneCodec.getProtocol()),
      &oneCodec);
  EXPECT_EQ(
      registry.getSerializerByName(thriftType("Int"), oneCodec.getProtocol()),
      &oneCodec);
  EXPECT_EQ(
      registry.getSerializerByName(
          thriftType("Integer"), oneCodec.getProtocol()),
      &oneCodec);

  auto any = cregistry.store(1, kFollyToStringProtocol);
  // Stored under the main type name.
  EXPECT_EQ(any.type_ref().value_or(""), thriftType("int"));
  EXPECT_EQ(cregistry.load<int>(any), 1);

  any.set_type(thriftType("Int"));
  EXPECT_EQ(cregistry.load<int>(any), 1);

  any.set_type(thriftType("Integer"));
  EXPECT_EQ(cregistry.load<int>(any), 1);

  any.set_type(thriftType("Unknown"));
  EXPECT_THROW(cregistry.load<int>(any), std::out_of_range);
}

TEST(AnyRegistryTest, ForwardCompat_Protocol) {
  AnyRegistry registry;
  Protocol invalidProtocol("invalid");
  EXPECT_THROW(validateProtocol(invalidProtocol), std::invalid_argument);
  // Lookup does not throw.
  EXPECT_EQ(registry.getSerializer<int>(invalidProtocol), nullptr);
}

TEST(AnyRegistryTest, ForwardCompat_Any) {
  AnyRegistry registry;
  const AnyRegistry& cregistry = registry;
  FollyToStringSerializer<int> intCodec;

  EXPECT_TRUE(registry.registerType<int>(testAnyType("int")));
  EXPECT_TRUE(registry.registerSerializer<int>(&intCodec));

  Any any = cregistry.store(1, kFollyToStringProtocol);

  validateAny(any);
  any.type_ref() = "invalid";
  EXPECT_THROW(validateAny(any), std::invalid_argument);
  // Load does not throw std::invalid_argument.
  EXPECT_THROW(cregistry.load(any), std::out_of_range);
}

TEST(AnyRegistryTest, StdProtocol) {
  AnyRegistry registry;
  const AnyRegistry& cregistry = registry;
  registry
      .registerType<Value, StandardProtocol::Binary, StandardProtocol::Compact>(
          testAnyType("Value"));

  auto value = asValueStruct<type::i32_t>(1);
  auto any = cregistry.store<StandardProtocol::Compact>(value);
  ASSERT_TRUE(any.type_ref());
  EXPECT_EQ(any.type_ref().value_unchecked(), thriftType("Value"));
  EXPECT_EQ(cregistry.load<Value>(any), value);
}

TEST(AnyRegistryTest, Generated) {
  detail::registerGeneratedStruct<
      Value,
      StandardProtocol::Binary,
      StandardProtocol::Compact>(testAnyType("Value"));

  // Double regeister fails with a runtime error.
  EXPECT_THROW(
      detail::registerGeneratedStruct<Value>(testAnyType("Value")),
      std::runtime_error);

  auto value = asValueStruct<type::i32_t>(1);
  auto any = AnyRegistry::generated().store<StandardProtocol::Compact>(value);
  ASSERT_TRUE(any.type_ref());
  EXPECT_EQ(any.type_ref().value_unchecked(), thriftType("Value"));
  EXPECT_EQ(AnyRegistry::generated().load<Value>(any), value);

  EXPECT_THROW(
      AnyRegistry::generated().store<StandardProtocol::Json>(value),
      std::out_of_range);
}

} // namespace
} // namespace apache::thrift::conformance
