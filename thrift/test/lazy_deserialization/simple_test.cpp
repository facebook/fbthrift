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
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/test/lazy_deserialization/MemberAccessor.h>
#include <thrift/test/lazy_deserialization/common.h>
#include <thrift/test/lazy_deserialization/gen-cpp2/simple_types.h>

namespace apache::thrift::test {

TYPED_TEST(LazyDeserialization, Copyable) {
  auto base = this->genLazyStruct();
  auto bar = base; // copy constructor
  typename TypeParam::LazyStruct baz;
  baz = base; // copy assignment

  EXPECT_EQ(bar.field1_ref(), baz.field1_ref());
  EXPECT_EQ(bar.field2_ref(), baz.field2_ref());
  EXPECT_EQ(bar.field3_ref(), baz.field3_ref());
  EXPECT_EQ(bar.field4_ref(), baz.field4_ref());

  auto foo = this->genStruct();

  EXPECT_EQ(foo.field1_ref(), bar.field1_ref());
  EXPECT_EQ(foo.field2_ref(), bar.field2_ref());
  EXPECT_EQ(foo.field3_ref(), bar.field3_ref());
  EXPECT_EQ(foo.field4_ref(), bar.field4_ref());
}

TYPED_TEST(LazyDeserialization, Moveable) {
  auto temp = this->genLazyStruct();
  auto bar = std::move(temp); // move constructor
  typename TypeParam::LazyStruct baz;
  baz = this->genLazyStruct(); // move assignment

  EXPECT_EQ(bar.field1_ref(), baz.field1_ref());
  EXPECT_EQ(bar.field2_ref(), baz.field2_ref());
  EXPECT_EQ(bar.field3_ref(), baz.field3_ref());
  EXPECT_EQ(bar.field4_ref(), baz.field4_ref());

  auto foo = this->genStruct();

  EXPECT_EQ(foo.field1_ref(), bar.field1_ref());
  EXPECT_EQ(foo.field2_ref(), bar.field2_ref());
  EXPECT_EQ(foo.field3_ref(), bar.field3_ref());
  EXPECT_EQ(foo.field4_ref(), bar.field4_ref());
}

template <typename T>
class Serialization : public testing::Test {};

using Serializers = ::testing::Types<CompactSerializer, BinarySerializer>;
TYPED_TEST_SUITE(Serialization, Serializers);

TYPED_TEST(LazyDeserialization, FooToLazyFoo) {
  using LazyStruct = typename TypeParam::LazyStruct;

  auto foo = this->genStruct();
  auto s = this->serialize(foo);
  auto lazyFoo = this->template deserialize<LazyStruct>(s);

  EXPECT_EQ(foo.field1_ref(), lazyFoo.field1_ref());
  EXPECT_EQ(foo.field2_ref(), lazyFoo.field2_ref());
  EXPECT_EQ(foo.field3_ref(), lazyFoo.field3_ref());
  EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
}

TYPED_TEST(LazyDeserialization, LazyFooToFoo) {
  using Struct = typename TypeParam::Struct;

  auto lazyFoo = this->genLazyStruct();
  auto s = this->serialize(lazyFoo);
  auto foo = this->template deserialize<Struct>(s);

  EXPECT_EQ(foo.field1_ref(), lazyFoo.field1_ref());
  EXPECT_EQ(foo.field2_ref(), lazyFoo.field2_ref());
  EXPECT_EQ(foo.field3_ref(), lazyFoo.field3_ref());
  EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
}

FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field1, LazyFoo, field1);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field2, LazyFoo, field2);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field3, LazyFoo, field3);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field4, LazyFoo, field4);

FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field1, OptionalLazyFoo, field1);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field2, OptionalLazyFoo, field2);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field3, OptionalLazyFoo, field3);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field4, OptionalLazyFoo, field4);

TYPED_TEST(LazyDeserialization, CheckDataMember) {
  using LazyStruct = typename TypeParam::LazyStruct;

  auto foo = this->genLazyStruct();
  auto s = this->serialize(foo);
  auto lazyFoo = this->template deserialize<LazyStruct>(s);

  EXPECT_EQ(get_field1(lazyFoo), foo.field1_ref());
  EXPECT_EQ(get_field2(lazyFoo), foo.field2_ref());
  EXPECT_TRUE(get_field3(lazyFoo).empty());
  EXPECT_TRUE(get_field4(lazyFoo).empty());

  EXPECT_EQ(lazyFoo.field1_ref(), foo.field1_ref());
  EXPECT_EQ(lazyFoo.field2_ref(), foo.field2_ref());
  EXPECT_EQ(lazyFoo.field3_ref(), foo.field3_ref());
  EXPECT_EQ(lazyFoo.field4_ref(), foo.field4_ref());

  EXPECT_EQ(get_field1(lazyFoo), foo.field1_ref());
  EXPECT_EQ(get_field2(lazyFoo), foo.field2_ref());
  EXPECT_EQ(get_field3(lazyFoo), foo.field3_ref());
  EXPECT_EQ(get_field4(lazyFoo), foo.field4_ref());
}

TYPED_TEST(Serialization, CppRef) {
  {
    LazyCppRef foo;
    foo.field1_ref() = std::make_unique<std::vector<int32_t>>(10, 10);
    foo.field2_ref() = std::make_shared<std::vector<int32_t>>(20, 20);
    foo.field3_ref() = std::make_shared<std::vector<int32_t>>(30, 30);
    auto s = TypeParam::template serialize<std::string>(foo);
    auto bar = TypeParam::template deserialize<LazyCppRef>(s);
    EXPECT_EQ(*bar.field1_ref(), std::vector<int32_t>(10, 10));
    EXPECT_EQ(*bar.field2_ref(), std::vector<int32_t>(20, 20));
    EXPECT_EQ(*bar.field3_ref(), std::vector<int32_t>(30, 30));
  }
  {
    LazyCppRef foo;
    auto s = TypeParam::template serialize<std::string>(foo);
    auto bar = TypeParam::template deserialize<LazyCppRef>(s);
    EXPECT_FALSE(bar.field1_ref());
    EXPECT_FALSE(bar.field2_ref());
    EXPECT_FALSE(bar.field3_ref());
  }
}

TYPED_TEST(LazyDeserialization, Comparison) {
  using LazyStruct = typename TypeParam::LazyStruct;

  {
    auto foo1 = this->genLazyStruct();
    auto s = this->serialize(foo1);
    auto foo2 = this->template deserialize<LazyStruct>(s);

    EXPECT_FALSE(get_field1(foo1).empty());
    EXPECT_FALSE(get_field2(foo1).empty());
    EXPECT_FALSE(get_field3(foo1).empty());
    EXPECT_FALSE(get_field4(foo1).empty());

    // field3 and field4 are lazy field, thus they are empty
    EXPECT_FALSE(get_field1(foo2).empty());
    EXPECT_FALSE(get_field2(foo2).empty());
    EXPECT_TRUE(get_field3(foo2).empty());
    EXPECT_TRUE(get_field4(foo2).empty());

    EXPECT_EQ(foo1, foo2);
    foo1.field4_ref()->clear();
    EXPECT_NE(foo1, foo2);
    foo2.field4_ref()->clear();
    EXPECT_EQ(foo1, foo2);
  }

  {
    auto foo1 = this->genLazyStruct();
    auto s = this->serialize(foo1);
    auto foo2 = this->template deserialize<LazyStruct>(s);

    foo1.field4_ref()->clear();

    EXPECT_LT(foo1, foo2);
  }
}

// This is testing if writer turned off lazy deserialization, but reader's
// lazyFoo has lazy field that's not deserialized, whether deserialization
// overwrites the field (i.e. whether we clear IOBuf and isDeserialized
// boolean).
TYPED_TEST(LazyDeserialization, Evolution) {
  using Struct = typename TypeParam::Struct;
  using LazyStruct = typename TypeParam::LazyStruct;

  Struct foo;
  foo.field1_ref().emplace();
  foo.field2_ref().emplace();
  foo.field3_ref().emplace();
  foo.field4_ref().emplace();

  // We need first deserialization since even though `this->genLazyStruct()`
  // returns lazy struct, all lazy fields are already deserialized.
  LazyStruct lazyFoo;
  this->deserialize(this->serialize(this->genLazyStruct()), lazyFoo);
  this->deserialize(this->serialize(foo), lazyFoo);

  EXPECT_TRUE(lazyFoo.field1_ref()->empty());
  EXPECT_TRUE(lazyFoo.field2_ref()->empty());
  EXPECT_TRUE(lazyFoo.field3_ref()->empty());
  EXPECT_TRUE(lazyFoo.field4_ref()->empty());
}

TYPED_TEST(LazyDeserialization, OptionalField) {
  using Struct = typename TypeParam::Struct;
  using LazyStruct = typename TypeParam::LazyStruct;

  auto s = this->serialize(this->genStruct());
  auto foo = this->template deserialize<Struct>(s);
  auto lazyFoo = this->template deserialize<LazyStruct>(s);

  EXPECT_EQ(foo.field1_ref(), lazyFoo.field1_ref());
  EXPECT_EQ(foo.field2_ref(), lazyFoo.field2_ref());
  EXPECT_EQ(foo.field3_ref(), lazyFoo.field3_ref());
  EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
}

TYPED_TEST(LazyDeserialization, ReserializeLazyField) {
  using LazyStruct = typename TypeParam::LazyStruct;

  auto foo1 = this->template deserialize<LazyStruct>(
      this->serialize(this->genLazyStruct()));
  auto foo2 = this->template deserialize<LazyStruct>(this->serialize(foo1));

  EXPECT_EQ(foo1.field1_ref(), foo2.field1_ref());
  EXPECT_EQ(foo1.field2_ref(), foo2.field2_ref());
  EXPECT_EQ(foo1.field3_ref(), foo2.field3_ref());
  EXPECT_EQ(foo1.field4_ref(), foo2.field4_ref());
}

TYPED_TEST(LazyDeserialization, SupportedToUnsupportedProtocol) {
  using LazyStruct = typename TypeParam::LazyStruct;

  auto foo1 = this->template deserialize<LazyStruct>(
      this->serialize(this->genLazyStruct()));

  EXPECT_FALSE(get_field1(foo1).empty());
  EXPECT_FALSE(get_field2(foo1).empty());
  EXPECT_TRUE(get_field3(foo1).empty());
  EXPECT_TRUE(get_field4(foo1).empty());

  // Simple JSON doesn't support lazy deserialization
  // All fields will be deserialized
  SimpleJSONSerializer::deserialize(
      SimpleJSONSerializer::serialize<std::string>(Empty{}), foo1);

  EXPECT_FALSE(get_field1(foo1).empty());
  EXPECT_FALSE(get_field2(foo1).empty());
  EXPECT_FALSE(get_field3(foo1).empty());
  EXPECT_FALSE(get_field4(foo1).empty());

  auto foo2 = this->genLazyStruct();

  EXPECT_EQ(foo1.field1_ref(), foo2.field1_ref());
  EXPECT_EQ(foo1.field2_ref(), foo2.field2_ref());
  EXPECT_EQ(foo1.field3_ref(), foo2.field3_ref());
  EXPECT_EQ(foo1.field4_ref(), foo2.field4_ref());
}

TYPED_TEST(LazyDeserialization, ReserializeSameStruct) {
  using LazyStruct = typename TypeParam::LazyStruct;

  auto foo1 = this->template deserialize<LazyStruct>(
      this->serialize(this->genLazyStruct()));

  EXPECT_FALSE(get_field1(foo1).empty());
  EXPECT_FALSE(get_field2(foo1).empty());
  EXPECT_TRUE(get_field3(foo1).empty());
  EXPECT_TRUE(get_field4(foo1).empty());

  // Lazy fields remain undeserialized
  this->deserialize(this->serialize(OptionalFoo{}), foo1);

  EXPECT_FALSE(get_field1(foo1).empty());
  EXPECT_FALSE(get_field2(foo1).empty());
  EXPECT_TRUE(get_field3(foo1).empty());
  EXPECT_TRUE(get_field4(foo1).empty());

  auto foo2 = this->genLazyStruct();

  EXPECT_EQ(foo1.field1_ref(), foo2.field1_ref());
  EXPECT_EQ(foo1.field2_ref(), foo2.field2_ref());
  EXPECT_EQ(foo1.field3_ref(), foo2.field3_ref());
  EXPECT_EQ(foo1.field4_ref(), foo2.field4_ref());
}

template <class Serializer1, class Serializer2>
void deserializationWithDifferentProtocol() {
  auto foo = Serializer1::template deserialize<LazyFoo>(
      Serializer1::template serialize<std::string>(gen<LazyFoo>()));

  EXPECT_FALSE(get_field1(foo).empty());
  EXPECT_FALSE(get_field2(foo).empty());
  EXPECT_TRUE(get_field3(foo).empty());
  EXPECT_TRUE(get_field4(foo).empty());

  // Deserialize with same protocol, all fields are untouched
  Serializer1::deserialize(
      Serializer1::template serialize<std::string>(OptionalFoo{}), foo);

  EXPECT_FALSE(get_field1(foo).empty());
  EXPECT_FALSE(get_field2(foo).empty());
  EXPECT_TRUE(get_field3(foo).empty());
  EXPECT_TRUE(get_field4(foo).empty());

  // Deserialize with different protocol, all fields are deserialized
  Serializer2::deserialize(
      Serializer2::template serialize<std::string>(OptionalFoo{}), foo);

  EXPECT_FALSE(get_field1(foo).empty());
  EXPECT_FALSE(get_field2(foo).empty());
  EXPECT_FALSE(get_field3(foo).empty());
  EXPECT_FALSE(get_field4(foo).empty());
}

TEST(Serialization, DeserializationWithDifferentProtocol) {
  deserializationWithDifferentProtocol<CompactSerializer, BinarySerializer>();
  deserializationWithDifferentProtocol<BinarySerializer, CompactSerializer>();
}

TEST(Serialization, SerializeWithDifferentProtocolSimple) {
  auto foo = CompactSerializer::deserialize<LazyFoo>(
      CompactSerializer::serialize<std::string>(gen<LazyFoo>()));

  auto foo1 = BinarySerializer::deserialize<LazyFoo>(
      BinarySerializer::serialize<std::string>(foo));

  EXPECT_EQ(foo1, gen<LazyFoo>());
}

template <class Serializer1, class Serializer2, class LazyStruct>
void serializationWithDifferentProtocol() {
  auto foo = Serializer1::template deserialize<LazyStruct>(
      Serializer1::template serialize<std::string>(gen<LazyStruct>()));

  auto foo1 = Serializer1::template deserialize<LazyStruct>(
      Serializer1::template serialize<std::string>(foo));

  // Serialize with same protocol, lazy fields won't be deserialized
  EXPECT_FALSE(get_field1(foo).empty());
  EXPECT_FALSE(get_field2(foo).empty());
  EXPECT_TRUE(get_field3(foo).empty());
  EXPECT_TRUE(get_field4(foo).empty());

  EXPECT_EQ(foo1, gen<LazyStruct>());

  auto foo2 = Serializer2::template deserialize<LazyStruct>(
      Serializer2::template serialize<std::string>(foo));

  // Serialize with different protocol, all fields are deserialized
  EXPECT_FALSE(get_field1(foo).empty());
  EXPECT_FALSE(get_field2(foo).empty());
  EXPECT_FALSE(get_field3(foo).empty());
  EXPECT_FALSE(get_field4(foo).empty());

  EXPECT_EQ(foo2, gen<LazyStruct>());
}

TEST(Serialization, SerializationWithDifferentProtocol) {
  serializationWithDifferentProtocol<
      CompactSerializer,
      BinarySerializer,
      LazyFoo>();
  serializationWithDifferentProtocol<
      BinarySerializer,
      CompactSerializer,
      LazyFoo>();
  serializationWithDifferentProtocol<
      CompactSerializer,
      BinarySerializer,
      OptionalLazyFoo>();
  serializationWithDifferentProtocol<
      BinarySerializer,
      CompactSerializer,
      OptionalLazyFoo>();
}

} // namespace apache::thrift::test
