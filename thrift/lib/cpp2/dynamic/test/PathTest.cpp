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

#include <thrift/lib/cpp2/dynamic/Path.h>
#include <thrift/lib/cpp2/dynamic/TypeSystemBuilder.h>
#include <thrift/lib/cpp2/op/Encode.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp2/type/Any.h>

#include <gtest/gtest.h>
#include <folly/io/IOBufQueue.h>

namespace apache::thrift::dynamic {
namespace {

using def = type_system::TypeSystemBuilder::DefinitionHelper;
using type_system::TypeIds;

template <typename T>
std::string encodeSimpleJSON(const T& value) {
  folly::IOBufQueue queue;
  SimpleJSONProtocolWriter writer;
  writer.setOutput(&queue);
  op::encode<type::infer_tag<T>>(writer, value);
  return queue.move()->to<std::string>();
}

// Helper to create container type caches
type_system::detail::ContainerTypeCache& containerCache() {
  static type_system::detail::ContainerTypeCache cache;
  return cache;
}

inline type_system::TypeRef::List makeListType(
    type_system::TypeRef elementType) {
  return type_system::TypeRef::List::of(elementType, containerCache());
}

inline type_system::TypeRef::Set makeSetType(type_system::TypeRef elementType) {
  return type_system::TypeRef::Set::of(elementType, containerCache());
}

inline type_system::TypeRef::Map makeMapType(
    type_system::TypeRef keyType, type_system::TypeRef valueType) {
  return type_system::TypeRef::Map::of(keyType, valueType, containerCache());
}

class PathTest : public ::testing::Test {
 protected:
  std::unique_ptr<type_system::TypeSystem> typeSystem;

  static constexpr auto kUserProfileUri = "meta.com/thrift/test/UserProfile";
  static constexpr auto kMyStructUri = "meta.com/thrift/test/MyStruct";
  static constexpr auto kRootUri = "meta.com/thrift/test/Root";
  static constexpr auto kInner1Uri = "meta.com/thrift/test/Inner1";
  static constexpr auto kInner2Uri = "meta.com/thrift/test/Inner2";
  static constexpr auto kBoolPrefixUri = "bool.example/thrift/test/boolRecord";
  static constexpr auto kListAliasUri = "meta.com/thrift/test/IntList";
  static constexpr auto kSetAliasUri = "meta.com/thrift/test/IntSet";
  static constexpr auto kMapAliasUri = "meta.com/thrift/test/IntMap";

  PathTest() {
    type_system::TypeSystemBuilder builder;

    // Create a nested struct for the users map value
    // struct UserProfile {
    //   1: string name
    //   2: list<i32> scores
    //   3: set<i32> tags
    //   4: any metadata
    // }
    builder.addType(
        kUserProfileUri,
        def::Struct({
            def::Field(
                def::Identity(1, "name"), def::AlwaysPresent, TypeIds::String),
            def::Field(
                def::Identity(2, "scores"),
                def::AlwaysPresent,
                TypeIds::list(TypeIds::I32)),
            def::Field(
                def::Identity(3, "tags"),
                def::AlwaysPresent,
                TypeIds::set(TypeIds::I32)),
            def::Field(
                def::Identity(4, "metadata"), def::Optional, TypeIds::Any),
        }));

    // Create the main struct
    // struct MyStruct {
    //   1: map<string, UserProfile> users
    //   2: map<i32, string> counts
    // }
    builder.addType(
        kMyStructUri,
        def::Struct({
            def::Field(
                def::Identity(1, "users"),
                def::AlwaysPresent,
                TypeIds::map(TypeIds::String, TypeIds::uri(kUserProfileUri))),
            def::Field(
                def::Identity(2, "counts"),
                def::AlwaysPresent,
                TypeIds::map(TypeIds::I32, TypeIds::String)),
        }));

    // Create a simple struct for basic tests
    builder.addType(
        kRootUri,
        def::Struct({
            def::Field(
                def::Identity(1, "a"),
                def::AlwaysPresent,
                TypeIds::uri(kInner1Uri)),
        }));

    builder.addType(
        kInner1Uri,
        def::Struct({
            def::Field(
                def::Identity(1, "b"),
                def::AlwaysPresent,
                TypeIds::uri(kInner2Uri)),
        }));

    builder.addType(
        kInner2Uri,
        def::Struct({
            def::Field(def::Identity(1, "c"), def::AlwaysPresent, TypeIds::I32),
        }));

    builder.addType(
        kBoolPrefixUri,
        def::Struct({def::Field(
            def::Identity(1, "value"), def::AlwaysPresent, TypeIds::I32)}));
    builder.addType(
        kListAliasUri, def::OpaqueAlias(TypeIds::list(TypeIds::I32)));
    builder.addType(kSetAliasUri, def::OpaqueAlias(TypeIds::set(TypeIds::I32)));
    builder.addType(
        kMapAliasUri,
        def::OpaqueAlias(TypeIds::map(TypeIds::I32, TypeIds::String)));

    typeSystem = std::move(builder).build();
  }

  type_system::TypeRef getMyStructType() {
    return type_system::TypeRef(
        typeSystem->getUserDefinedTypeOrThrow(kMyStructUri).asStruct());
  }

  type_system::TypeRef getUserProfileType() {
    return type_system::TypeRef(
        typeSystem->getUserDefinedTypeOrThrow(kUserProfileUri).asStruct());
  }

  type_system::TypeRef getRootType() {
    return type_system::TypeRef(
        typeSystem->getUserDefinedTypeOrThrow(kRootUri).asStruct());
  }

  type_system::TypeRef getBoolPrefixType() {
    return type_system::TypeRef(
        typeSystem->getUserDefinedTypeOrThrow(kBoolPrefixUri).asStruct());
  }

  type_system::TypeRef getType(std::string_view uri) {
    return type_system::TypeRef(
        typeSystem->getUserDefinedTypeOrThrow(uri).asOpaqueAlias());
  }

  void expectRoundTrip(
      type_system::TypeRef rootType,
      std::string_view serialized,
      type_system::TypeRef finalType,
      std::size_t componentCount) {
    SCOPED_TRACE(serialized);
    try {
      auto parsed = PathBuilder::fromString(*typeSystem, rootType, serialized);
      EXPECT_EQ(parsed.toString(), serialized);
      EXPECT_EQ(parsed.currentType().id(), finalType.id());
      EXPECT_EQ(parsed.path().size(), componentCount);
    } catch (const std::exception& error) {
      ADD_FAILURE() << "Unexpected exception: " << error.what();
    }
  }

  void expectInvalidPath(
      type_system::TypeRef rootType, std::string_view serialized) {
    SCOPED_TRACE(serialized);
    EXPECT_THROW(
        (void)PathBuilder::fromString(*typeSystem, rootType, serialized),
        InvalidPathAccessError);
  }

  void expectInvalidType(
      type_system::TypeRef rootType, std::string_view serialized) {
    SCOPED_TRACE(serialized);
    EXPECT_THROW(
        (void)PathBuilder::fromString(*typeSystem, rootType, serialized),
        type_system::InvalidTypeError);
  }
};

// Tests for Path via PathBuilder

TEST_F(PathTest, PathBasics) {
  PathBuilder builder(getMyStructType());

  EXPECT_EQ(builder.path().toString(), "MyStruct");

  {
    auto g1 = builder.enterField("users");
    EXPECT_EQ(builder.path().toString(), "MyStruct.users");

    {
      auto g2 = builder.enterMapValue("alice");
      EXPECT_EQ(builder.path().toString(), "MyStruct.users[\"alice\"]");

      {
        auto g3 = builder.enterField(FieldId{2});
        auto g4 = builder.enterListElement(0);
        EXPECT_EQ(
            builder.path().toString(), "MyStruct.users[\"alice\"].scores[0]");
      }

      EXPECT_EQ(builder.path().toString(), "MyStruct.users[\"alice\"]");
    }
  }

  EXPECT_EQ(builder.path().toString(), "MyStruct");
}

TEST_F(PathTest, PathAllComponentTypes) {
  PathBuilder builder(getMyStructType());

  {
    auto g1 = builder.enterField("users");
    EXPECT_EQ(builder.path().toString(), "MyStruct.users");
  }

  {
    auto g1 = builder.enterField("users");
    auto g2 = builder.enterMapValue("alice");
    auto g3 = builder.enterField("scores");
    auto g4 = builder.enterListElement(42);
    EXPECT_EQ(
        builder.path().toString(), "MyStruct.users[\"alice\"].scores[42]");
  }

  {
    auto g1 = builder.enterField("users");
    auto g2 = builder.enterMapValue("alice");
    auto g3 = builder.enterField("tags");
    auto g4 = builder.enterSetElement(123);
    EXPECT_EQ(builder.path().toString(), "MyStruct.users[\"alice\"].tags{123}");
  }

  {
    auto g1 = builder.enterField("users");
    auto g2 = builder.enterMapKey("key");
    EXPECT_EQ(builder.path().toString(), "MyStruct.users{\"key\"}");
  }

  {
    auto g1 = builder.enterField("users");
    auto g2 = builder.enterMapValue("alice");
    auto g3 = builder.enterField("metadata");
    auto g4 = builder.enterAnyType(getUserProfileType());
    EXPECT_EQ(
        builder.path().toString(),
        "MyStruct.users[\"alice\"].metadata[meta.com/thrift/test/UserProfile]");
  }
}

TEST_F(PathTest, PathFormatsSelectorTypes) {
  auto stringKeyPath = [&] {
    PathBuilder builder(getMyStructType());
    auto field = builder.enterField("users");
    auto value = builder.enterMapValue("42");
    return builder.path();
  }();
  EXPECT_EQ(stringKeyPath.toString(), "MyStruct.users[\"42\"]");

  auto integerKeyPath = [&] {
    PathBuilder builder(getMyStructType());
    auto field = builder.enterField("counts");
    auto value = builder.enterMapValue(42);
    return builder.path();
  }();
  EXPECT_EQ(integerKeyPath.toString(), "MyStruct.counts[42]");

  auto anyTypePath = [&] {
    PathBuilder builder(getMyStructType());
    auto users = builder.enterField("users");
    auto user = builder.enterMapValue("alice");
    auto metadata = builder.enterField("metadata");
    auto type = builder.enterAnyType(getUserProfileType());
    return builder.path();
  }();
  EXPECT_EQ(
      anyTypePath.toString(),
      "MyStruct.users[\"alice\"].metadata[meta.com/thrift/test/UserProfile]");
}

TEST_F(PathTest, InspectComponents) {
  Path path = [&] {
    PathBuilder builder(getMyStructType());
    auto users = builder.enterField("users");
    auto user = builder.enterMapValue("alice");
    auto metadata = builder.enterField("metadata");
    auto type = builder.enterAnyType(getUserProfileType());
    auto scores = builder.enterField("scores");
    auto score = builder.enterListElement(2);
    return builder.path();
  }();

  EXPECT_EQ(path.rootType().id(), getMyStructType().id());
  EXPECT_FALSE(path.empty());
  ASSERT_EQ(path.size(), 6);

  const auto* users = std::get_if<Path::FieldAccess>(&path.components()[0]);
  ASSERT_NE(users, nullptr);
  EXPECT_EQ(users->structuredType().id(), getMyStructType().id());
  EXPECT_EQ(users->fieldId(), FieldId{1});
  EXPECT_EQ(users->fieldName(), "users");
  EXPECT_TRUE(users->fieldType().isMap());

  const auto* user = std::get_if<Path::MapValue>(&path.components()[1]);
  ASSERT_NE(user, nullptr);
  EXPECT_EQ(user->key(), DynamicValue::makeString("alice"));

  const auto* type = std::get_if<Path::AnyType>(&path.components()[3]);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->type().id(), getUserProfileType().id());

  const auto* score = std::get_if<Path::ListElement>(&path.components()[5]);
  ASSERT_NE(score, nullptr);
  EXPECT_EQ(score->index(), 2);

  Path mapKeyPath = [&] {
    PathBuilder builder(getMyStructType());
    auto counts = builder.enterField("counts");
    auto key = builder.enterMapKey(42);
    return builder.path();
  }();
  const auto* mapKey =
      std::get_if<Path::MapKey>(&mapKeyPath.components().back());
  ASSERT_NE(mapKey, nullptr);
  EXPECT_EQ(mapKey->key(), DynamicValue::makeI32(42));

  auto setType =
      type_system::TypeRef(makeSetType(type_system::TypeSystem::String()));
  Path setPath = [&] {
    PathBuilder builder(setType);
    auto element = builder.enterSetElement("value");
    return builder.path();
  }();
  const auto* setElement =
      std::get_if<Path::SetElement>(&setPath.components().front());
  ASSERT_NE(setElement, nullptr);
  EXPECT_EQ(setElement->value(), DynamicValue::makeString("value"));
}

TEST_F(PathTest, BuilderStructuredMapKey) {
  auto mapType = type_system::TypeRef(makeMapType(
      type_system::TypeSystem::Any(), type_system::TypeSystem::String()));
  PathBuilder builder(mapType);
  auto key = type::AnyData::toAny<type::i32_t>(42).toThrift();

  auto guard = builder.enterMapKey(key);

  EXPECT_EQ(
      builder.toString(),
      fmt::format("map<any, string>{{{}}}", encodeSimpleJSON(key)));
}
// Tests for PathBuilder (typed builder with validation)

TEST_F(PathTest, BuilderAllAccessTypes) {
  PathBuilder builder(getMyStructType());

  // Empty path should just be the root type name
  EXPECT_EQ(builder.toString(), "MyStruct");

  {
    // Enter a field
    auto g1 = builder.enterField("users");
    EXPECT_EQ(builder.toString(), "MyStruct.users");

    {
      // Enter a map value with string key
      auto g2 = builder.enterMapValue("alice");
      EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"]");

      {
        // Enter a nested field
        auto g3 = builder.enterField("scores");
        EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"].scores");

        {
          // Enter a list element by index
          auto g4 = builder.enterListElement(0);
          EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"].scores[0]");
        }

        // After scope guard destruction, back to scores
        EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"].scores");
      }

      // Back to user profile
      EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"]");

      {
        // Enter the tags field (a set)
        auto g3 = builder.enterField("tags");
        EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"].tags");

        {
          // Enter a set element with int value
          auto g4 = builder.enterSetElement(42);
          EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"].tags{42}");
        }

        EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"].tags");
      }

      // Test enterAnyType with known type
      {
        auto g3 = builder.enterField("metadata");
        EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"].metadata");

        {
          auto g4 = builder.enterAnyType(getUserProfileType());
          EXPECT_EQ(
              builder.toString(),
              "MyStruct.users[\"alice\"].metadata[meta.com/thrift/test/UserProfile]");

          // Can navigate further since type is known
          {
            auto g5 = builder.enterField("name");
            EXPECT_EQ(
                builder.toString(),
                "MyStruct.users[\"alice\"].metadata[meta.com/thrift/test/UserProfile].name");
          }
        }

        EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"].metadata");
      }
    }

    // Back to users field
    EXPECT_EQ(builder.toString(), "MyStruct.users");

    {
      // Enter a map key with string key
      auto g2 = builder.enterMapKey("bob");
      EXPECT_EQ(builder.toString(), "MyStruct.users{\"bob\"}");
    }
  }

  // Back to root
  EXPECT_EQ(builder.toString(), "MyStruct");

  // Test with integer map keys
  {
    auto g1 = builder.enterField("counts");
    EXPECT_EQ(builder.toString(), "MyStruct.counts");

    {
      auto g2 = builder.enterMapKey(123);
      EXPECT_EQ(builder.toString(), "MyStruct.counts{123}");
    }
  }

  // Final check: back to root
  EXPECT_EQ(builder.toString(), "MyStruct");
}

TEST_F(PathTest, BuilderNestedScopes) {
  PathBuilder builder(getRootType());

  {
    auto g1 = builder.enterField("a");
    {
      auto g2 = builder.enterField("b");
      {
        auto g3 = builder.enterField("c");
        EXPECT_EQ(builder.toString(), "Root.a.b.c");
      }
      EXPECT_EQ(builder.toString(), "Root.a.b");
    }
    EXPECT_EQ(builder.toString(), "Root.a");
  }
  EXPECT_EQ(builder.toString(), "Root");
}

TEST_F(PathTest, BuilderTypeContext) {
  PathBuilder builder(getMyStructType());
  auto users = builder.enterField("users");
  {
    auto value = builder.enterTypeContext(getUserProfileType());
    auto name = builder.enterField("name");
    EXPECT_EQ(builder.toString(), "MyStruct.users.name");
  }
  EXPECT_EQ(builder.toString(), "MyStruct.users");
}

TEST_F(PathTest, BuilderExportPath) {
  PathBuilder builder(getMyStructType());

  auto g1 = builder.enterField("users");
  auto g2 = builder.enterMapValue("alice");
  auto g3 = builder.enterField("scores");
  auto g4 = builder.enterListElement(0);

  Path exported = builder.path();
  EXPECT_EQ(exported.toString(), "MyStruct.users[\"alice\"].scores[0]");
}

TEST_F(PathTest, BuilderInvalidFieldAccess) {
  PathBuilder builder(getMyStructType());

  // Accessing a non-existent field should throw
  EXPECT_THROW((void)builder.enterField("nonexistent"), InvalidPathAccessError);

  // Accessing a field on a non-structured type should throw
  {
    auto g1 = builder.enterField("users");
    auto g2 = builder.enterMapValue("alice");
    auto g3 = builder.enterField("scores");
    auto g4 = builder.enterListElement(0);
    // Now current type is i32
    EXPECT_THROW((void)builder.enterField("foo"), InvalidPathAccessError);
  }
}

TEST_F(PathTest, BuilderInvalidListAccess) {
  PathBuilder builder(getMyStructType());

  // Accessing list element on non-list type should throw
  EXPECT_THROW((void)builder.enterListElement(0), InvalidPathAccessError);
}

TEST_F(PathTest, BuilderInvalidSetAccess) {
  PathBuilder builder(getMyStructType());

  // Accessing set element on non-set type should throw
  EXPECT_THROW((void)builder.enterSetElement(42), InvalidPathAccessError);
}

TEST_F(PathTest, BuilderInvalidMapAccess) {
  PathBuilder builder(getMyStructType());

  // Accessing map key on non-map type should throw
  EXPECT_THROW((void)builder.enterMapKey("key"), InvalidPathAccessError);

  // Accessing map value on non-map type should throw
  EXPECT_THROW((void)builder.enterMapValue("key"), InvalidPathAccessError);
}

TEST_F(PathTest, BuilderRejectsMismatchedDynamicSelector) {
  auto mapType = type_system::TypeRef(makeMapType(
      type_system::TypeSystem::I32(), type_system::TypeSystem::String()));
  PathBuilder builder(mapType);

  EXPECT_THROW(
      (void)builder.enterMapValue(DynamicValue::makeString("42")),
      InvalidPathAccessError);

  auto stringMapType = type_system::TypeRef(makeMapType(
      type_system::TypeSystem::String(), type_system::TypeSystem::String()));
  PathBuilder stringBuilder(stringMapType);
  EXPECT_THROW((void)stringBuilder.enterMapValue(42), InvalidPathAccessError);
}

TEST_F(PathTest, BuilderInvalidAnyAccess) {
  PathBuilder builder(getMyStructType());

  // Accessing any type on non-any type should throw
  EXPECT_THROW(
      (void)builder.enterAnyType(getUserProfileType()), InvalidPathAccessError);
}

TEST_F(PathTest, BuilderAnyType) {
  PathBuilder builder(getMyStructType());

  auto g1 = builder.enterField("users");
  auto g2 = builder.enterMapValue("alice");
  auto g3 = builder.enterField("metadata");

  // Current type is any
  EXPECT_TRUE(builder.currentType().isAny());

  {
    // Enter any type with known inner type (UserProfile)
    auto g4 = builder.enterAnyType(getUserProfileType());
    EXPECT_EQ(
        builder.toString(),
        "MyStruct.users[\"alice\"].metadata[meta.com/thrift/test/UserProfile]");

    // Current type is now UserProfile
    EXPECT_TRUE(builder.currentType().isStruct());

    // Further typed access should work since the inner type is known
    {
      auto g5 = builder.enterField("name");
      EXPECT_EQ(
          builder.toString(),
          "MyStruct.users[\"alice\"].metadata[meta.com/thrift/test/UserProfile].name");
      EXPECT_TRUE(builder.currentType().isString());
    }

    // Can also access other fields
    {
      auto g5 = builder.enterField("scores");
      EXPECT_TRUE(builder.currentType().isList());

      {
        auto g6 = builder.enterListElement(0);
        EXPECT_TRUE(builder.currentType().isI32());
      }
    }

    // Invalid field still throws
    EXPECT_THROW(
        (void)builder.enterField("nonexistent"), InvalidPathAccessError);
  }

  // Back to metadata
  EXPECT_EQ(builder.toString(), "MyStruct.users[\"alice\"].metadata");
  EXPECT_TRUE(builder.currentType().isAny());
}

TEST_F(PathTest, BuilderCurrentType) {
  PathBuilder builder(getMyStructType());

  // Initially, current type is the root struct
  EXPECT_TRUE(builder.currentType().isStruct());

  {
    auto g1 = builder.enterField("users");
    // After entering users field, current type is map<string, UserProfile>
    EXPECT_TRUE(builder.currentType().isMap());

    {
      auto g2 = builder.enterMapValue("alice");
      // After entering map value, current type is UserProfile
      EXPECT_TRUE(builder.currentType().isStruct());

      {
        auto g3 = builder.enterField("scores");
        // After entering scores field, current type is list<i32>
        EXPECT_TRUE(builder.currentType().isList());

        {
          auto g4 = builder.enterListElement(0);
          // After entering list element, current type is i32
          EXPECT_TRUE(builder.currentType().isI32());
        }

        // Back to list
        EXPECT_TRUE(builder.currentType().isList());
      }

      // Back to UserProfile
      EXPECT_TRUE(builder.currentType().isStruct());
    }

    // Back to map
    EXPECT_TRUE(builder.currentType().isMap());
  }

  // Back to root struct
  EXPECT_TRUE(builder.currentType().isStruct());
}

TEST_F(PathTest, BuilderPrimitiveTypes) {
  // Test with primitive types
  PathBuilder boolBuilder(type_system::TypeSystem::Bool());
  EXPECT_EQ(boolBuilder.toString(), "bool");

  PathBuilder i32Builder(type_system::TypeSystem::I32());
  EXPECT_EQ(i32Builder.toString(), "i32");

  PathBuilder stringBuilder(type_system::TypeSystem::String());
  EXPECT_EQ(stringBuilder.toString(), "string");
}

TEST_F(PathTest, BuilderContainerTypes) {
  // Test with container types
  auto listType =
      type_system::TypeRef(makeListType(type_system::TypeSystem::I32()));
  PathBuilder listBuilder(listType);
  EXPECT_EQ(listBuilder.toString(), "list<i32>");

  {
    auto g = listBuilder.enterListElement(5);
    EXPECT_EQ(listBuilder.toString(), "list<i32>[5]");
    EXPECT_TRUE(listBuilder.currentType().isI32());
  }

  auto setType =
      type_system::TypeRef(makeSetType(type_system::TypeSystem::String()));
  PathBuilder setBuilder(setType);
  EXPECT_EQ(setBuilder.toString(), "set<string>");

  {
    auto g = setBuilder.enterSetElement("hello");
    EXPECT_EQ(setBuilder.toString(), "set<string>{\"hello\"}");
    // After entering set element, current type is the element type
    EXPECT_TRUE(setBuilder.currentType().isString());
  }

  auto mapType = type_system::TypeRef(makeMapType(
      type_system::TypeSystem::I32(), type_system::TypeSystem::String()));
  PathBuilder mapBuilder(mapType);
  EXPECT_EQ(mapBuilder.toString(), "map<i32, string>");

  {
    auto g = mapBuilder.enterMapValue(42);
    EXPECT_EQ(mapBuilder.toString(), "map<i32, string>[42]");
    EXPECT_TRUE(mapBuilder.currentType().isString());
  }

  {
    auto g = mapBuilder.enterMapKey(42);
    EXPECT_EQ(mapBuilder.toString(), "map<i32, string>{42}");
    // After entering map key, current type is the key type
    EXPECT_TRUE(mapBuilder.currentType().isI32());
  }
}

TEST_F(PathTest, ParsePrimitiveRootTypesRoundTrip) {
  expectRoundTrip(
      type_system::TypeSystem::Bool(),
      "bool",
      type_system::TypeSystem::Bool(),
      0);
  expectRoundTrip(
      type_system::TypeSystem::Byte(),
      "byte",
      type_system::TypeSystem::Byte(),
      0);
  expectRoundTrip(
      type_system::TypeSystem::I16(), "i16", type_system::TypeSystem::I16(), 0);
  expectRoundTrip(
      type_system::TypeSystem::I32(), "i32", type_system::TypeSystem::I32(), 0);
  expectRoundTrip(
      type_system::TypeSystem::I64(), "i64", type_system::TypeSystem::I64(), 0);
  expectRoundTrip(
      type_system::TypeSystem::Float(),
      "float",
      type_system::TypeSystem::Float(),
      0);
  expectRoundTrip(
      type_system::TypeSystem::Double(),
      "double",
      type_system::TypeSystem::Double(),
      0);
  expectRoundTrip(
      type_system::TypeSystem::String(),
      "string",
      type_system::TypeSystem::String(),
      0);
  expectRoundTrip(
      type_system::TypeSystem::Binary(),
      "binary",
      type_system::TypeSystem::Binary(),
      0);
  expectRoundTrip(
      type_system::TypeSystem::Any(), "any", type_system::TypeSystem::Any(), 0);
}

TEST_F(PathTest, ParseEveryComponentTypeRoundTrips) {
  const auto myStructType = getMyStructType();
  expectRoundTrip(myStructType, "MyStruct", myStructType, 0);
  const auto usersType =
      myStructType.asStruct()
          .at(myStructType.asStruct().fieldHandleFor("users"))
          .type();
  expectRoundTrip(myStructType, "MyStruct.users", usersType, 1);
  expectRoundTrip(
      getMyStructType(), "MyStruct.users[\"alice\"]", getUserProfileType(), 2);
  expectRoundTrip(
      getMyStructType(),
      "MyStruct.users{\"alice\"}",
      type_system::TypeSystem::String(),
      2);
  expectRoundTrip(
      getMyStructType(),
      "MyStruct.users[\"alice\"].scores[0]",
      type_system::TypeSystem::I32(),
      4);
  expectRoundTrip(
      getMyStructType(),
      "MyStruct.users[\"alice\"].tags{-42}",
      type_system::TypeSystem::I32(),
      4);
  expectRoundTrip(
      getMyStructType(),
      "MyStruct.users[\"alice\"].metadata[meta.com/thrift/test/UserProfile].name",
      type_system::TypeSystem::String(),
      5);
}

TEST_F(PathTest, ParsePopulatesComponentsWithOptionalRootTypeName) {
  auto expectComponents = [&](std::string_view serialized) {
    SCOPED_TRACE(serialized);
    auto builder =
        PathBuilder::fromString(*typeSystem, getMyStructType(), serialized);
    auto path = builder.path();

    EXPECT_EQ(path.rootType().id(), getMyStructType().id());
    EXPECT_EQ(builder.currentType().id(), TypeIds::I32);
    ASSERT_EQ(path.size(), 6);

    const auto& components = path.components();
    const auto* users = std::get_if<Path::FieldAccess>(&components[0]);
    ASSERT_NE(users, nullptr);
    EXPECT_EQ(users->structuredType().id(), getMyStructType().id());
    EXPECT_EQ(users->fieldId(), FieldId{1});
    EXPECT_EQ(users->fieldName(), "users");

    const auto* user = std::get_if<Path::MapValue>(&components[1]);
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->key(), DynamicValue::makeString("alice"));

    const auto* metadata = std::get_if<Path::FieldAccess>(&components[2]);
    ASSERT_NE(metadata, nullptr);
    EXPECT_EQ(metadata->structuredType().id(), getUserProfileType().id());
    EXPECT_EQ(metadata->fieldId(), FieldId{4});
    EXPECT_EQ(metadata->fieldName(), "metadata");

    const auto* anyType = std::get_if<Path::AnyType>(&components[3]);
    ASSERT_NE(anyType, nullptr);
    EXPECT_EQ(anyType->type().id(), getUserProfileType().id());

    const auto* scores = std::get_if<Path::FieldAccess>(&components[4]);
    ASSERT_NE(scores, nullptr);
    EXPECT_EQ(scores->structuredType().id(), getUserProfileType().id());
    EXPECT_EQ(scores->fieldId(), FieldId{2});
    EXPECT_EQ(scores->fieldName(), "scores");

    const auto* score = std::get_if<Path::ListElement>(&components[5]);
    ASSERT_NE(score, nullptr);
    EXPECT_EQ(score->index(), 2);
  };

  constexpr std::string_view components =
      R"(.users["alice"].metadata[meta.com/thrift/test/UserProfile].scores[2])";
  expectComponents(fmt::format("MyStruct{}", components));
  expectComponents(components);
}

TEST_F(PathTest, ParseUriStartingWithPrimitiveNameRoundTrips) {
  expectRoundTrip(
      getBoolPrefixType(),
      "boolRecord.value",
      type_system::TypeSystem::I32(),
      1);
  expectRoundTrip(
      type_system::TypeSystem::Any(),
      "any[bool.example/thrift/test/boolRecord].value",
      type_system::TypeSystem::I32(),
      2);
}

TEST_F(PathTest, ParseOpaqueAliasContainerTraversalRoundTrips) {
  expectRoundTrip(
      getType(kListAliasUri), "IntList[2]", type_system::TypeSystem::I32(), 1);
  expectRoundTrip(
      getType(kSetAliasUri), "IntSet{3}", type_system::TypeSystem::I32(), 1);
  expectRoundTrip(
      getType(kMapAliasUri), "IntMap[4]", type_system::TypeSystem::String(), 1);
}

TEST_F(PathTest, ParseContainerRootTypesRoundTrip) {
  const auto listType =
      type_system::TypeRef(makeListType(type_system::TypeSystem::I32()));
  expectRoundTrip(listType, "list<i32>[42]", type_system::TypeSystem::I32(), 1);

  const auto setType =
      type_system::TypeRef(makeSetType(type_system::TypeSystem::String()));
  expectRoundTrip(
      setType, "set<string>{\"hello\"}", type_system::TypeSystem::String(), 1);

  const auto mapType = type_system::TypeRef(makeMapType(
      type_system::TypeSystem::I32(), type_system::TypeSystem::String()));
  expectRoundTrip(
      mapType, "map<i32, string>{-42}", type_system::TypeSystem::I32(), 1);
  expectRoundTrip(
      mapType, "map<i32, string>[42]", type_system::TypeSystem::String(), 1);
}

TEST_F(PathTest, ParseNestedContainerTypeNamesRoundTrip) {
  const auto setType =
      type_system::TypeRef(makeSetType(type_system::TypeSystem::I64()));
  const auto listType = type_system::TypeRef(makeListType(setType));
  const auto mapType = type_system::TypeRef(
      makeMapType(type_system::TypeSystem::String(), listType));

  expectRoundTrip(
      mapType,
      "map<string, list<set<i64>>>[\"key\"][3]{-1}",
      type_system::TypeSystem::I64(),
      3);
}

TEST_F(PathTest, ParseSimpleJsonSelectorsWithPathDelimitersRoundTrip) {
  const auto mapType = type_system::TypeRef(makeMapType(
      type_system::TypeSystem::String(), type_system::TypeSystem::I32()));
  expectRoundTrip(
      mapType,
      R"(map<string, i32>["a.b[0]{\"x\"}"])",
      type_system::TypeSystem::I32(),
      1);
}

TEST_F(PathTest, ParseRejectsInvalidStructuredTraversal) {
  expectInvalidPath(getMyStructType(), "MyStruct.");
  expectInvalidPath(getMyStructType(), "MyStruct.unknown");
  expectInvalidPath(getMyStructType(), "MyStruct.users.");
  expectInvalidPath(getMyStructType(), "MyStruct.users[\"alice\"]name");
  expectInvalidPath(getMyStructType(), "MyStruct.users-name");
}

TEST_F(PathTest, ParseRejectsInvalidListTraversal) {
  const auto listType =
      type_system::TypeRef(makeListType(type_system::TypeSystem::I32()));
  expectInvalidPath(listType, "list<i32>[]");
  expectInvalidPath(listType, "list<i32>[-1]");
  expectInvalidPath(listType, "list<i32>[abc]");
  expectInvalidPath(listType, "list<i32>[1");
  expectInvalidPath(listType, "list<i32>[1]trailing");
  expectInvalidPath(listType, "list<i32>[184467440737095516160]");
}

TEST_F(PathTest, ParseRejectsInvalidSetTraversal) {
  const auto setType =
      type_system::TypeRef(makeSetType(type_system::TypeSystem::String()));
  expectInvalidPath(setType, "set<string>[\"value\"]");
  expectInvalidPath(setType, "set<string>{\"value\"]");
  expectInvalidPath(setType, "set<string>{not-json}");
  expectInvalidPath(setType, "set<string>{\"value");
  expectInvalidPath(setType, "set<string>{\"value\"}trailing");
}

TEST_F(PathTest, ParseRejectsInvalidMapTraversal) {
  const auto mapType = type_system::TypeRef(makeMapType(
      type_system::TypeSystem::I32(), type_system::TypeSystem::String()));
  expectInvalidPath(mapType, "map<i32, string>(42)");
  expectInvalidPath(mapType, "map<i32, string>{\"wrong type\"}");
  expectInvalidPath(mapType, "map<i32, string>[42");
  expectInvalidPath(mapType, "map<i32, string>{42]");
  expectInvalidPath(mapType, "map<i32, string>[42]trailing");
}

TEST_F(PathTest, ParseRejectsMalformedContainerTypeNames) {
  const auto listType =
      type_system::TypeRef(makeListType(type_system::TypeSystem::I32()));
  expectInvalidPath(listType, "list<i32[0]");
  expectInvalidPath(listType, "list<list<i32>[0]");

  const auto mapType = type_system::TypeRef(makeMapType(
      type_system::TypeSystem::I32(), type_system::TypeSystem::String()));
  expectInvalidPath(mapType, "map<i32 string>[42]");
  expectInvalidPath(mapType, "map<i32, string[42]");
}

TEST_F(PathTest, ParseRejectsInvalidAnyTraversal) {
  expectInvalidPath(type_system::TypeSystem::Any(), "any[]");
  expectInvalidType(
      type_system::TypeSystem::Any(), "any[unknown.example/Type]");
  expectInvalidPath(
      type_system::TypeSystem::Any(), "any[meta.com/thrift/test/UserProfile");
  expectInvalidPath(type_system::TypeSystem::Any(), "any[list<i32,junk>]");
  expectInvalidPath(
      type_system::TypeSystem::Any(),
      "any[meta.com/thrift/test/UserProfile]trailing");
}

TEST_F(PathTest, ParseRejectsTraversalFromPrimitiveType) {
  expectInvalidPath(type_system::TypeSystem::Bool(), "bool.value");
  expectInvalidPath(type_system::TypeSystem::I32(), "i32[0]");
  expectInvalidPath(type_system::TypeSystem::String(), "string{\"value\"}");
}

} // namespace
} // namespace apache::thrift::dynamic
