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

#include <thrift/lib/cpp2/dynamic/Serialization.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp2/dynamic/DynamicValue.h>
#include <thrift/lib/cpp2/dynamic/test/gen-cpp2/json5_serialization_test_types.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Json5Protocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/runtime/SchemaRegistry.h>

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace apache::thrift::dynamic {
namespace {

namespace gen = json5_test;
using json5::detail::Json5ProtocolWriter;
using json5::detail::JsonWriterOptions;
using testing::HasSubstr;

template <typename T>
type_system::TypeRef typeOf() {
  return SchemaRegistry::get().getTypeSystemNode<T>().asRef();
}

// Converts through Compact, which the dynamic path already supports.
template <typename T>
DynamicValue toDynamic(const T& value) {
  auto buf = CompactSerializer::serialize<folly::IOBufQueue>(value).move();
  CompactProtocolReader reader;
  reader.setInput(buf.get());
  return deserializeValue(reader, typeOf<T>());
}

Json5ProtocolWriter::Options json5Options() {
  auto writer = json5::detail::kJson5Options;
  writer.indentWidth = 2;
  return {.writer = writer};
}

// Each of these changes how some values are encoded.
std::vector<Json5ProtocolWriter::Options> allWriterOptions() {
  std::vector<Json5ProtocolWriter::Options> ret;
  for (const auto& writer :
       {JsonWriterOptions{},
        JsonWriterOptions{.indentWidth = 2},
        json5Options().writer}) {
    ret.push_back({.writer = writer});
    ret.push_back({.writer = writer, .enumAsInteger = true});
    ret.push_back({.writer = writer, .binaryAsBase64String = true});
    ret.push_back({.writer = writer, .mapPrimitiveKeysAsMemberNames = true});
    ret.push_back(
        {.writer = writer,
         .binaryAsBase64String = true,
         .mapPrimitiveKeysAsMemberNames = true});
  }
  return ret;
}

std::string toJson(
    const DynamicConstRef& value,
    const Json5ProtocolWriter::Options& options = json5Options()) {
  folly::IOBufQueue queue;
  Json5ProtocolWriter writer(COPY_EXTERNAL_BUFFER, options);
  writer.setOutput(&queue);
  serializeValue(writer, value);
  return queue.moveAsValue().toString();
}

template <typename T>
void expectSameJsonAsStatic(const T& value) {
  const auto dynamicValue = toDynamic(value);
  for (const auto& options : allWriterOptions()) {
    EXPECT_EQ(
        toJson(dynamicValue, options),
        json5::detail::toJsonImpl<type::infer_tag<T>>(value, options));
  }
}

gen::Key makeKey(std::int32_t first, std::int32_t second) {
  gen::Key key;
  key.first() = first;
  key.second() = second;
  return key;
}

gen::Choice makeNumberChoice(std::int32_t number) {
  gen::Choice choice;
  choice.number() = number;
  return choice;
}

gen::Choice makeTextChoice(std::string text) {
  gen::Choice choice;
  choice.text() = std::move(text);
  return choice;
}

gen::Choice makeColorChoice(gen::Color color) {
  gen::Choice choice;
  choice.color() = color;
  return choice;
}

gen::Scalars makeScalars() {
  gen::Scalars scalars;
  scalars.boolField() = true;
  scalars.byteField() = -7;
  scalars.i32Field() = 32;
  scalars.i16Field() = -16;
  scalars.i64Field() = std::numeric_limits<std::int64_t>::max();
  scalars.floatField() = 1.5f;
  scalars.doubleField() = -0.0;
  scalars.stringField() = R"(hello "world")";
  scalars.binaryField() = std::string("\x00\xff\x10", 3);
  scalars.enumField() = gen::Color::BLUE;
  scalars.optionalField() = "set";
  return scalars;
}

TEST(Json5SerializationTest, WritesStructFieldsInFieldIdOrder) {
  const auto key = makeKey(1, 2);
  EXPECT_EQ(toJson(toDynamic(key)), "{\n  first: 1,\n  second: 2,\n}");
  expectSameJsonAsStatic(key);
}

TEST(Json5SerializationTest, WritesEnumsByName) {
  gen::Containers containers;
  containers.enumList() = {gen::Color::RED, gen::Color::BLUE};
  containers.enumSet() = {gen::Color::GREEN};
  containers.enumKeyMap() = {{gen::Color::RED, "r"}};
  containers.enumValueMap() = {{"b", gen::Color::BLUE}};
  containers.unionList() = {makeColorChoice(gen::Color::GREEN)};

  const auto json = toJson(toDynamic(containers));
  EXPECT_THAT(json, HasSubstr("\"RED (1)\""));
  EXPECT_THAT(json, HasSubstr("\"GREEN (2)\""));
  EXPECT_THAT(json, HasSubstr("\"BLUE (3)\""));
  expectSameJsonAsStatic(containers);
}

TEST(Json5SerializationTest, WritesTopLevelEnumByName) {
  const auto value = DynamicValue::makeEnum(
      SchemaRegistry::get().getTypeSystemNode<gen::Color>(), 2);
  EXPECT_THAT(toJson(value), HasSubstr("GREEN (2)"));
  EXPECT_EQ(
      toJson(value),
      Json5ProtocolUtils::toJson5<type::enum_t<gen::Color>>(gen::Color::GREEN));
}

TEST(Json5SerializationTest, WritesUnknownEnumValueAsParenthesizedNumber) {
  auto scalars = makeScalars();
  scalars.enumField() = static_cast<gen::Color>(99);
  EXPECT_THAT(toJson(toDynamic(scalars)), HasSubstr("\"(99)\""));
  expectSameJsonAsStatic(scalars);
}

TEST(Json5SerializationTest, WritesNonStringMapKeysAsKeyValueArray) {
  gen::Containers containers;
  containers.intKeyMap() = {{1, "a"}};
  containers.structKeyMap() = {{makeKey(1, 2), "k"}};
  containers.binaryKeyMap() = {{"bin", 3}};
  EXPECT_THAT(toJson(toDynamic(containers)), HasSubstr("key: 1,"));
  expectSameJsonAsStatic(containers);
}

TEST(Json5SerializationTest, WritesScalarsLikeStaticPath) {
  expectSameJsonAsStatic(makeScalars());

  auto special = makeScalars();
  special.floatField() = std::numeric_limits<float>::quiet_NaN();
  special.doubleField() = -std::numeric_limits<double>::infinity();
  special.optionalField().reset();
  expectSameJsonAsStatic(special);
}

TEST(Json5SerializationTest, WritesUnionsLikeStaticPath) {
  expectSameJsonAsStatic(makeNumberChoice(7));
  expectSameJsonAsStatic(makeTextChoice("seven"));
  expectSameJsonAsStatic(makeColorChoice(gen::Color::RED));
  expectSameJsonAsStatic(gen::Choice{});
}

TEST(Json5SerializationTest, WritesSingleElementContainersLikeStaticPath) {
  gen::AllTypes all;
  all.scalars() = makeScalars();
  auto& containers = *all.containers();
  containers.intList() = {3, 1, 2};
  containers.enumList() = {gen::Color::BLUE, gen::Color::RED};
  containers.unionList() = {makeColorChoice(gen::Color::RED)};
  containers.stringSet() = {"only"};
  containers.intSet() = {-5};
  containers.binarySet() = {"b"};
  containers.enumSet() = {gen::Color::RED};
  containers.structSet() = {makeKey(3, 4)};
  containers.unionSet() = {makeColorChoice(gen::Color::BLUE)};
  containers.listSet() = {{1, 2}};
  containers.stringMap() = {{"a", 1}};
  containers.intKeyMap() = {{-1, "neg"}};
  containers.enumKeyMap() = {{gen::Color::GREEN, "g"}};
  containers.structKeyMap() = {{makeKey(5, 6), "k"}};
  containers.binaryKeyMap() = {{"bk", 9}};
  containers.enumValueMap() = {{"v", gen::Color::RED}};
  all.choice()->text() = "chosen";
  all.key() = makeKey(7, 8);
  expectSameJsonAsStatic(all);
}

} // namespace
} // namespace apache::thrift::dynamic
