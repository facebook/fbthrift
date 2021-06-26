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
#include <thrift/test/lazy_deserialization/gen-cpp2/simple_types.h>

namespace apache::thrift::test {

template <class Struct>
Struct gen() {
  Struct obj;
  obj.field1_ref().emplace(10, 1);
  obj.field2_ref().emplace(20, 2);
  obj.field3_ref().emplace(30, 3);
  obj.field4_ref().emplace(40, 4);
  return obj;
}

TEST(Serialization, Copyable) {
  auto base = gen<LazyFoo>();
  auto bar = base; // copy constructor
  LazyFoo baz;
  baz = base; // copy assignment

  EXPECT_EQ(bar.field1_ref(), baz.field1_ref());
  EXPECT_EQ(bar.field2_ref(), baz.field2_ref());
  EXPECT_EQ(bar.field3_ref(), baz.field3_ref());
  EXPECT_EQ(bar.field4_ref(), baz.field4_ref());

  auto foo = gen<Foo>();

  EXPECT_EQ(foo.field1_ref(), bar.field1_ref());
  EXPECT_EQ(foo.field2_ref(), bar.field2_ref());
  EXPECT_EQ(foo.field3_ref(), bar.field3_ref());
  EXPECT_EQ(foo.field4_ref(), bar.field4_ref());
}

TEST(Serialization, Moveable) {
  auto temp = gen<LazyFoo>();
  auto bar = std::move(temp); // move constructor
  LazyFoo baz;
  baz = gen<LazyFoo>(); // move assignment

  EXPECT_EQ(bar.field1_ref(), baz.field1_ref());
  EXPECT_EQ(bar.field2_ref(), baz.field2_ref());
  EXPECT_EQ(bar.field3_ref(), baz.field3_ref());
  EXPECT_EQ(bar.field4_ref(), baz.field4_ref());

  auto foo = gen<Foo>();

  EXPECT_EQ(foo.field1_ref(), bar.field1_ref());
  EXPECT_EQ(foo.field2_ref(), bar.field2_ref());
  EXPECT_EQ(foo.field3_ref(), bar.field3_ref());
  EXPECT_EQ(foo.field4_ref(), bar.field4_ref());
}

template <typename T>
class Serialization : public testing::Test {};

using Serializers = ::testing::Types<CompactSerializer, BinarySerializer>;
TYPED_TEST_SUITE(Serialization, Serializers);

TYPED_TEST(Serialization, FooToLazyFoo) {
  auto foo = gen<Foo>();
  auto s = TypeParam::template serialize<std::string>(foo);
  auto lazyFoo = TypeParam::template deserialize<LazyFoo>(s);

  EXPECT_EQ(foo.field1_ref(), lazyFoo.field1_ref());
  EXPECT_EQ(foo.field2_ref(), lazyFoo.field2_ref());
  EXPECT_EQ(foo.field3_ref(), lazyFoo.field3_ref());
  EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
}

TYPED_TEST(Serialization, LazyFooToFoo) {
  auto lazyFoo = gen<LazyFoo>();
  auto s = TypeParam::template serialize<std::string>(lazyFoo);
  auto foo = TypeParam::template deserialize<Foo>(s);

  EXPECT_EQ(foo.field1_ref(), lazyFoo.field1_ref());
  EXPECT_EQ(foo.field2_ref(), lazyFoo.field2_ref());
  EXPECT_EQ(foo.field3_ref(), lazyFoo.field3_ref());
  EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
}

FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field1, LazyFoo, field1);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field2, LazyFoo, field2);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field3, LazyFoo, field3);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field4, LazyFoo, field4);

TYPED_TEST(Serialization, CheckDataMember) {
  auto foo = gen<LazyFoo>();
  auto s = TypeParam::template serialize<std::string>(foo);
  auto lazyFoo = TypeParam::template deserialize<LazyFoo>(s);

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

TYPED_TEST(Serialization, Comparison) {
  {
    auto foo1 = gen<LazyFoo>();
    auto s = TypeParam::template serialize<std::string>(foo1);
    auto foo2 = TypeParam::template deserialize<LazyFoo>(s);

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
    auto foo1 = gen<LazyFoo>();
    auto s = TypeParam::template serialize<std::string>(foo1);
    auto foo2 = TypeParam::template deserialize<LazyFoo>(s);

    foo1.field4_ref()->clear();

    EXPECT_LT(foo1, foo2);
  }
}

TYPED_TEST(Serialization, Evolution) {
  LazyFoo foo;
  TypeParam::deserialize(
      TypeParam::template serialize<std::string>(gen<LazyFoo>()), foo);
  TypeParam::deserialize(
      TypeParam::template serialize<std::string>(Foo()), foo);
  EXPECT_TRUE(foo.field1_ref()->empty());
  EXPECT_TRUE(foo.field2_ref()->empty());
  EXPECT_TRUE(foo.field3_ref()->empty());
  EXPECT_TRUE(foo.field4_ref()->empty());
}

TYPED_TEST(Serialization, OptionalField) {
  auto s = TypeParam::template serialize<std::string>(gen<Foo>());
  auto foo = TypeParam::template deserialize<OptionalFoo>(s);
  auto lazyFoo = TypeParam::template deserialize<OptionalLazyFoo>(s);

  EXPECT_EQ(foo.field1_ref(), lazyFoo.field1_ref());
  EXPECT_EQ(foo.field2_ref(), lazyFoo.field2_ref());
  EXPECT_EQ(foo.field3_ref(), lazyFoo.field3_ref());
  EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
}

TYPED_TEST(Serialization, ReserializeLazyField) {
  auto foo1 = TypeParam::template deserialize<LazyFoo>(
      TypeParam::template serialize<std::string>(gen<LazyFoo>()));
  auto foo2 = TypeParam::template deserialize<LazyFoo>(
      TypeParam::template serialize<std::string>(foo1));

  EXPECT_EQ(foo1.field1_ref(), foo2.field1_ref());
  EXPECT_EQ(foo1.field2_ref(), foo2.field2_ref());
  EXPECT_EQ(foo1.field3_ref(), foo2.field3_ref());
  EXPECT_EQ(foo1.field4_ref(), foo2.field4_ref());
}

TYPED_TEST(Serialization, SupportedToUnsupportedProtocol) {
  auto foo1 = TypeParam::template deserialize<LazyFoo>(
      TypeParam::template serialize<std::string>(gen<LazyFoo>()));

  EXPECT_FALSE(get_field1(foo1).empty());
  EXPECT_FALSE(get_field2(foo1).empty());
  EXPECT_TRUE(get_field3(foo1).empty());
  EXPECT_TRUE(get_field4(foo1).empty());

  // Simple JSON doesn't support lazy deserialization
  // All fields will be deserialized
  SimpleJSONSerializer::deserialize(
      SimpleJSONSerializer::serialize<std::string>(OptionalFoo{}), foo1);

  EXPECT_FALSE(get_field1(foo1).empty());
  EXPECT_FALSE(get_field2(foo1).empty());
  EXPECT_FALSE(get_field3(foo1).empty());
  EXPECT_FALSE(get_field4(foo1).empty());

  auto foo2 = gen<LazyFoo>();

  EXPECT_EQ(foo1.field1_ref(), foo2.field1_ref());
  EXPECT_EQ(foo1.field2_ref(), foo2.field2_ref());
  EXPECT_EQ(foo1.field3_ref(), foo2.field3_ref());
  EXPECT_EQ(foo1.field4_ref(), foo2.field4_ref());
}

TYPED_TEST(Serialization, ReserializeSameStruct) {
  auto foo1 = TypeParam::template deserialize<LazyFoo>(
      TypeParam::template serialize<std::string>(gen<LazyFoo>()));

  EXPECT_FALSE(get_field1(foo1).empty());
  EXPECT_FALSE(get_field2(foo1).empty());
  EXPECT_TRUE(get_field3(foo1).empty());
  EXPECT_TRUE(get_field4(foo1).empty());

  // Lazy fields remain undeserialized
  TypeParam::deserialize(
      TypeParam::template serialize<std::string>(OptionalFoo{}), foo1);

  EXPECT_FALSE(get_field1(foo1).empty());
  EXPECT_FALSE(get_field2(foo1).empty());
  EXPECT_TRUE(get_field3(foo1).empty());
  EXPECT_TRUE(get_field4(foo1).empty());

  auto foo2 = gen<LazyFoo>();

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

} // namespace apache::thrift::test
