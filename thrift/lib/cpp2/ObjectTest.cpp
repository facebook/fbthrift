/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/lib/cpp2/Object.h>

#include <set>

#include <folly/io/IOBufQueue.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/conformance/cpp2/AnyRegistry.h>
#include <thrift/conformance/cpp2/AnyStructSerializer.h>
#include <thrift/conformance/cpp2/Protocol.h>
#include <thrift/conformance/cpp2/Testing.h>
#include <thrift/conformance/cpp2/internal/AnyStructSerializer.h>
#include <thrift/conformance/data/ValueGenerator.h>
#include <thrift/conformance/if/gen-cpp2/conformance_types_custom_protocol.h>
#include <thrift/conformance/if/gen-cpp2/protocol_types_custom_protocol.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/test/testset/Testset.h>
#include <thrift/test/testset/gen-cpp2/testset_types_custom_protocol.h>

// TODO: Remove this. Specify namespace explicitly instead.
using namespace ::apache::thrift::conformance;

using detail::protocol_reader_t;
using detail::protocol_writer_t;

namespace apache::thrift::protocol {
namespace {

namespace testset = apache::thrift::test::testset;

template <typename C>
decltype(auto) at(C& container, size_t i) {
  auto itr = container.begin();
  std::advance(itr, i);
  return *itr;
}

TEST(ObjectTest, TypeEnforced) {
  // Always a bool when bool_t is used, without ambiguity.
  // Pointers implicitly converts to bools.
  Value value = asValueStruct<type::bool_t>("");
  ASSERT_EQ(value.getType(), Value::boolValue);
  EXPECT_TRUE(value.get_boolValue());
}

TEST(ObjectTest, Bool) {
  Value value = asValueStruct<type::bool_t>(20);
  ASSERT_EQ(value.getType(), Value::boolValue);
  EXPECT_TRUE(value.get_boolValue());

  value = asValueStruct<type::bool_t>(0);
  ASSERT_EQ(value.getType(), Value::boolValue);
  EXPECT_FALSE(value.get_boolValue());
}

TEST(ObjectTest, Byte) {
  Value value = asValueStruct<type::byte_t>(7u);
  ASSERT_EQ(value.getType(), Value::byteValue);
  EXPECT_EQ(value.get_byteValue(), 7);
}

TEST(ObjectTest, I16) {
  Value value = asValueStruct<type::i16_t>(7u);
  ASSERT_EQ(value.getType(), Value::i16Value);
  EXPECT_EQ(value.get_i16Value(), 7);
}

TEST(ObjectTest, I32) {
  Value value = asValueStruct<type::i32_t>(7u);
  ASSERT_EQ(value.getType(), Value::i32Value);
  EXPECT_EQ(value.get_i32Value(), 7);
}

TEST(ObjectTest, I64) {
  Value value = asValueStruct<type::i64_t>(7u);
  ASSERT_EQ(value.getType(), Value::i64Value);
  EXPECT_EQ(value.get_i64Value(), 7);
}

TEST(ObjectTest, Enum) {
  enum class MyEnum { kValue = 7 };
  Value value = asValueStruct<type::enum_c>(MyEnum::kValue);
  ASSERT_EQ(value.getType(), Value::i32Value);
  EXPECT_EQ(value.get_i32Value(), 7);

  value = asValueStruct<type::enum_c>(static_cast<MyEnum>(2));
  ASSERT_EQ(value.getType(), Value::i32Value);
  EXPECT_EQ(value.get_i32Value(), 2);

  value = asValueStruct<type::enum_c>(21u);
  ASSERT_EQ(value.getType(), Value::i32Value);
  EXPECT_EQ(value.get_i32Value(), 21);
}

TEST(ObjectTest, Float) {
  Value value = asValueStruct<type::float_t>(1.5);
  ASSERT_EQ(value.getType(), Value::floatValue);
  EXPECT_EQ(value.get_floatValue(), 1.5f);
}

TEST(ObjectTest, Double) {
  Value value = asValueStruct<type::double_t>(1.5f);
  ASSERT_EQ(value.getType(), Value::doubleValue);
  EXPECT_EQ(value.get_doubleValue(), 1.5);
}

TEST(ObjectTest, String) {
  Value value = asValueStruct<type::string_t>("hi");
  ASSERT_EQ(value.getType(), Value::stringValue);
  EXPECT_EQ(value.get_stringValue(), "hi");
}

TEST(ObjectTest, Binary) {
  Value value = asValueStruct<type::binary_t>("hi");
  ASSERT_EQ(value.getType(), Value::binaryValue);
  EXPECT_EQ(toString(value.get_binaryValue()), "hi");
}

TEST(ObjectTest, List) {
  std::vector<int> data = {1, 4, 2};
  Value value = asValueStruct<type::list<type::i16_t>>(data);
  ASSERT_EQ(value.getType(), Value::listValue);
  ASSERT_EQ(value.get_listValue().size(), data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    EXPECT_EQ(value.get_listValue()[i], asValueStruct<type::i16_t>(data[i]));
  }

  // Works with other containers
  value = asValueStruct<type::list<type::i16_t>>(
      std::set<int>(data.begin(), data.end()));
  std::sort(data.begin(), data.end());
  ASSERT_EQ(value.getType(), Value::listValue);
  ASSERT_EQ(value.get_listValue().size(), data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    EXPECT_EQ(value.get_listValue()[i], asValueStruct<type::i16_t>(data[i]));
  }
}

TEST(ObjectTest, List_Move) {
  // Validate the premise of the test.
  std::string s1 = "hi";
  std::string s2 = std::move(s1);
  EXPECT_EQ(s1, "");
  EXPECT_EQ(s2, "hi");

  std::vector<std::string> data;
  data.emplace_back("hi");
  data.emplace_back("bye");

  Value value = asValueStruct<type::list<type::string_t>>(data);
  // The strings are unchanged
  EXPECT_THAT(data, ::testing::ElementsAre("hi", "bye"));
  ASSERT_EQ(value.getType(), Value::listValue);
  ASSERT_EQ(value.get_listValue().size(), 2);
  EXPECT_EQ(value.get_listValue()[0].get_stringValue(), "hi");
  EXPECT_EQ(value.get_listValue()[1].get_stringValue(), "bye");

  value = asValueStruct<type::list<type::string_t>>(std::move(data));

  // The strings have been moved.
  EXPECT_THAT(data, ::testing::ElementsAre("", ""));
  ASSERT_EQ(value.getType(), Value::listValue);
  ASSERT_EQ(value.get_listValue().size(), 2);
  EXPECT_EQ(value.get_listValue()[0].get_stringValue(), "hi");
  EXPECT_EQ(value.get_listValue()[1].get_stringValue(), "bye");
}

TEST(ObjectTest, Set) {
  std::set<int> data = {1, 4, 2};
  Value value = asValueStruct<type::set<type::i16_t>>(data);
  ASSERT_EQ(value.getType(), Value::setValue);
  ASSERT_EQ(value.get_setValue().size(), data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    EXPECT_EQ(
        at(value.get_setValue(), i), asValueStruct<type::i16_t>(at(data, i)));
  }

  // Works with other containers
  value = asValueStruct<type::set<type::i16_t>>(
      std::vector<int>(data.begin(), data.end()));
  ASSERT_EQ(value.getType(), Value::setValue);
  ASSERT_EQ(value.get_setValue().size(), data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    EXPECT_EQ(
        at(value.get_setValue(), i), asValueStruct<type::i16_t>(at(data, i)));
  }
}

TEST(ObjectTest, Map) {
  std::map<std::string, int> data = {{"one", 1}, {"four", 4}, {"two", 2}};
  Value value = asValueStruct<type::map<type::string_t, type::byte_t>>(data);
  ASSERT_EQ(value.getType(), Value::mapValue);
  ASSERT_EQ(value.get_mapValue().size(), data.size());
  for (const auto& entry : data) {
    auto itr =
        value.get_mapValue().find(asValueStruct<type::string_t>(entry.first));
    ASSERT_NE(itr, value.get_mapValue().end());
    EXPECT_EQ(itr->second, asValueStruct<type::byte_t>(entry.second));
  }

  // Works with other containers.
  std::vector<std::pair<std::string, int>> otherData(data.begin(), data.end());
  value = asValueStruct<type::map<type::string_t, type::byte_t>>(otherData);
  ASSERT_EQ(value.getType(), Value::mapValue);
  ASSERT_EQ(value.get_mapValue().size(), data.size());
  for (const auto& entry : data) {
    auto itr =
        value.get_mapValue().find(asValueStruct<type::string_t>(entry.first));
    ASSERT_NE(itr, value.get_mapValue().end());
    EXPECT_EQ(itr->second, asValueStruct<type::byte_t>(entry.second));
  }
}

TEST(ObjectTest, Struct) {
  // TODO(afuller): Use a struct that covers more cases.
  auto protocol = ::apache::thrift::conformance::Protocol("hi").asStruct();
  Value value = asValueStruct<type::union_c>(protocol);
  ASSERT_EQ(value.getType(), Value::objectValue);
  const Object& object = value.get_objectValue();
  EXPECT_EQ(object.members_ref()->size(), 2);
  EXPECT_EQ(
      object.members_ref()->at(1),
      asValueStruct<type::enum_c>(
          ::apache::thrift::conformance::StandardProtocol::Custom));
  EXPECT_EQ(object.members_ref()->at(2), asValueStruct<type::binary_t>("hi"));
}

TEST(ObjectTest, StructWithList) {
  testset::struct_with<type::list<type::i32_t>> s;
  std::vector<int> listValues = {1, 2, 3};
  s.field_1_ref() = listValues;
  Value value = asValueStruct<type::struct_c>(s);
  ASSERT_EQ(value.getType(), Value::objectValue);
  const Object& object = value.get_objectValue();
  EXPECT_EQ(object.members_ref()->size(), 1);
  EXPECT_EQ(
      object.members_ref()->at(1),
      asValueStruct<type::list<type::i32_t>>(listValues));
}

TEST(ObjectTest, StructWithMap) {
  testset::struct_with<type::map<type::string_t, type::i32_t>> s;
  std::map<std::string, int> mapValues = {{"one", 1}, {"four", 4}, {"two", 2}};
  s.field_1_ref() = mapValues;
  Value value = asValueStruct<type::struct_c>(s);
  ASSERT_EQ(value.getType(), Value::objectValue);
  const Object& object = value.get_objectValue();
  EXPECT_EQ(object.members_ref()->size(), 1);
  auto val = asValueStruct<type::map<type::binary_t, type::i32_t>>(mapValues);
  EXPECT_EQ(object.members_ref()->at(1), val);
}

TEST(ObjectTest, StructWithSet) {
  testset::struct_with<type::set<type::i64_t>> s;
  std::set<long> setValues = {1, 2, 3};
  s.field_1_ref() = setValues;
  Value value = asValueStruct<type::struct_c>(s);
  ASSERT_EQ(value.getType(), Value::objectValue);
  const Object& object = value.get_objectValue();
  EXPECT_EQ(object.members_ref()->size(), 1);
  EXPECT_EQ(
      object.members_ref()->at(1),
      asValueStruct<type::set<type::i64_t>>(setValues));
}

TEST(ObjectTest, parseObject) {
  folly::IOBufQueue iobufQueue;
  testset::struct_with<type::set<type::i64_t>> thriftStruct;
  std::set<long> setValues = {1, 2, 3};
  thriftStruct.field_1_ref() = setValues;
  BinarySerializer::serialize(thriftStruct, &iobufQueue);
  auto serialized = iobufQueue.move();
  auto object = parseObject<BinarySerializer::ProtocolReader>(*serialized);
  EXPECT_EQ(object.members_ref()->size(), 1);
  EXPECT_EQ(
      object.members_ref()->at(1),
      asValueStruct<type::set<type::i64_t>>(setValues));
}

TEST(ObjectTest, serializeObject) {
  folly::IOBufQueue iobufQueue;
  testset::struct_with<type::set<type::i64_t>> thriftStruct;
  std::set<long> setValues = {1, 2, 3};
  thriftStruct.field_1_ref() = setValues;
  BinarySerializer::serialize(thriftStruct, &iobufQueue);
  auto expected = iobufQueue.move();
  auto object = parseObject<BinarySerializer::ProtocolReader>(*expected);
  auto actual = serializeObject<BinarySerializer::ProtocolWriter>(object);
  EXPECT_TRUE(folly::IOBufEqualTo{}(*actual, *expected));
}

TEST(ObjectTest, ValueUnionTypeMatch) {
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::boolValue),
      type::BaseType::Bool);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::byteValue),
      type::BaseType::Byte);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::i16Value), type::BaseType::I16);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::i32Value), type::BaseType::I32);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::i64Value), type::BaseType::I64);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::floatValue),
      type::BaseType::Float);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::doubleValue),
      type::BaseType::Double);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::stringValue),
      type::BaseType::String);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::binaryValue),
      type::BaseType::Binary);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::listValue),
      type::BaseType::List);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::setValue), type::BaseType::Set);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::mapValue), type::BaseType::Map);
  EXPECT_EQ(
      static_cast<type::BaseType>(Value::Type::objectValue),
      type::BaseType::Struct);
}

template <typename ParseObjectTestCase>
class TypedParseObjectTest : public testing::Test {};

template <::apache::thrift::conformance::StandardProtocol Protocol, typename T>
std::unique_ptr<folly::IOBuf> serialize(T& s) {
  folly::IOBufQueue iobufQueue;
  protocol_writer_t<Protocol> writer;
  writer.setOutput(&iobufQueue);
  s.write(&writer);
  auto iobuf = iobufQueue.move();
  return iobuf;
}

template <
    ::apache::thrift::conformance::StandardProtocol Protocol,
    typename Tag,
    typename T>
void testParseObject() {
  T testsetValue;
  for (const auto& val : data::ValueGenerator<Tag>::getKeyValues()) {
    SCOPED_TRACE(val.name);
    testsetValue.field_1_ref() = val.value;
    auto valueStruct = asValueStruct<type::struct_c>(testsetValue);
    const Object& object = valueStruct.get_objectValue();

    auto iobuf = serialize<Protocol, T>(testsetValue);
    auto objFromParseObject = parseObject<protocol_reader_t<Protocol>>(*iobuf);
    EXPECT_EQ(objFromParseObject, object);
  }
}

template <typename Tag>
bool hasEmptyContainer(const type::standard_type<Tag>& value) {
  if constexpr (type::is_a_v<Tag, type::container_c>) {
    if (value.size() == 0) {
      return true;
    }
  }
  if constexpr (type::is_a_v<Tag, type::map<type::all_c, type::container_c>>) {
    for (auto const& [mapkey, mapval] : value) {
      if (mapval.size() == 0) {
        return true;
      }
    }
  }
  return false;
}

template <
    ::apache::thrift::conformance::StandardProtocol Protocol,
    typename Tag,
    typename T>
void testSerializeObject() {
  T testsetValue;
  for (const auto& val : data::ValueGenerator<Tag>::getKeyValues()) {
    SCOPED_TRACE(val.name);
    testsetValue.field_1_ref() = val.value;
    auto valueStruct = asValueStruct<type::struct_c>(testsetValue);
    const Object& object = valueStruct.get_objectValue();
    auto schemalessSerializedData =
        serializeObject<protocol_writer_t<Protocol>>(object);

    auto schemaBasedSerializedData = serialize<Protocol, T>(testsetValue);

    // FIXME: skip validation for structs with empty list, set, map.
    if (hasEmptyContainer<Tag>(val.value)) {
      continue;
    }
    EXPECT_TRUE(folly::IOBufEqualTo{}(
        *schemalessSerializedData, *schemaBasedSerializedData));
  }
}

template <
    ::apache::thrift::conformance::StandardProtocol Protocol,
    typename Tag,
    typename T>
void testSerializeObjectAny() {
  AnyRegistry registry;
  registry.registerType<T>(
      createThriftTypeInfo({"facebook.com/thrift/conformance/struct_with"}));
  registry.registerSerializer<T>(&getAnyStandardSerializer<T, Protocol>());
  for (const auto& val : data::ValueGenerator<Tag>::getKeyValues()) {
    SCOPED_TRACE(val.name);
    RoundTripResponse anyValue;
    T testsetValue;
    testsetValue.field_1_ref() = val.value;
    anyValue.value_ref() = registry.store<Protocol>(testsetValue);

    auto schemaBasedSerializedData = serialize<Protocol>(anyValue);
    auto objFromParseObject =
        parseObject<protocol_reader_t<Protocol>>(*schemaBasedSerializedData);

    auto schemalessSerializedData =
        serializeObject<protocol_writer_t<Protocol>>(objFromParseObject);

    EXPECT_TRUE(folly::IOBufEqualTo{}(
        *schemalessSerializedData, *schemaBasedSerializedData));
  }
}

// The tests cases to run.
using ParseObjectTestCases = ::testing::Types<
    type::bool_t,
    type::byte_t,
    type::i16_t,
    type::i32_t,
    type::i64_t,
    type::float_t,
    type::double_t,
    type::binary_t,
    type::string_t,
    type::list<type::i64_t>,
    type::list<type::string_t>,
    type::set<type::i64_t>,
    type::set<type::string_t>,
    type::map<type::string_t, type::i64_t>,
    type::map<type::i64_t, type::double_t>,
    type::map<type::i64_t, type::set<type::string_t>>>;

TYPED_TEST_SUITE(TypedParseObjectTest, ParseObjectTestCases);

TYPED_TEST(TypedParseObjectTest, ParseSerializedSameAsDirectObject) {
  testParseObject<
      ::apache::thrift::conformance::StandardProtocol::Binary,
      TypeParam,
      testset::struct_with<TypeParam>>();
  testParseObject<
      ::apache::thrift::conformance::StandardProtocol::Compact,
      TypeParam,
      testset::struct_with<TypeParam>>();
  testParseObject<
      ::apache::thrift::conformance::StandardProtocol::Binary,
      TypeParam,
      testset::union_with<TypeParam>>();
  testParseObject<
      ::apache::thrift::conformance::StandardProtocol::Compact,
      TypeParam,
      testset::union_with<TypeParam>>();
}

TYPED_TEST(TypedParseObjectTest, SerializeObjectSameAsDirectSerialization) {
  testSerializeObject<
      ::apache::thrift::conformance::StandardProtocol::Binary,
      TypeParam,
      testset::struct_with<TypeParam>>();
  testSerializeObject<
      ::apache::thrift::conformance::StandardProtocol::Compact,
      TypeParam,
      testset::struct_with<TypeParam>>();
  testSerializeObject<
      ::apache::thrift::conformance::StandardProtocol::Binary,
      TypeParam,
      testset::union_with<TypeParam>>();
  testSerializeObject<
      ::apache::thrift::conformance::StandardProtocol::Compact,
      TypeParam,
      testset::union_with<TypeParam>>();
}

TYPED_TEST(TypedParseObjectTest, SerializeObjectSameAsDirectSerializationAny) {
  testSerializeObjectAny<
      ::apache::thrift::conformance::StandardProtocol::Binary,
      TypeParam,
      testset::struct_with<TypeParam>>();
  testSerializeObjectAny<
      ::apache::thrift::conformance::StandardProtocol::Compact,
      TypeParam,
      testset::struct_with<TypeParam>>();
  testSerializeObjectAny<
      ::apache::thrift::conformance::StandardProtocol::Binary,
      TypeParam,
      testset::union_with<TypeParam>>();
  testSerializeObjectAny<
      ::apache::thrift::conformance::StandardProtocol::Compact,
      TypeParam,
      testset::union_with<TypeParam>>();
}

} // namespace
} // namespace apache::thrift::protocol
