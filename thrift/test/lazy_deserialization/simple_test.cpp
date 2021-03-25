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
  auto foo = gen<LazyFoo>();
  LazyFoo bar = foo, baz;
  baz = foo;

  EXPECT_EQ(bar.field1_ref(), baz.field1_ref());
  EXPECT_EQ(bar.field2_ref(), baz.field2_ref());
  EXPECT_EQ(bar.field3_ref(), baz.field3_ref());
  EXPECT_EQ(bar.field4_ref(), baz.field4_ref());
}

TEST(Serialization, FooToLazyFoo) {
  auto foo = gen<Foo>();
  auto s = CompactSerializer::serialize<std::string>(foo);
  auto lazyFoo = CompactSerializer::deserialize<LazyFoo>(s);

  EXPECT_EQ(foo.field1_ref(), lazyFoo.field1_ref());
  EXPECT_EQ(foo.field2_ref(), lazyFoo.field2_ref());
  EXPECT_EQ(foo.field3_ref(), lazyFoo.field3_ref());
  EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
}

TEST(Serialization, LazyFooToFoo) {
  auto lazyFoo = gen<LazyFoo>();
  auto s = CompactSerializer::serialize<std::string>(lazyFoo);
  auto foo = CompactSerializer::deserialize<Foo>(s);

  EXPECT_EQ(foo.field1_ref(), lazyFoo.field1_ref());
  EXPECT_EQ(foo.field2_ref(), lazyFoo.field2_ref());
  EXPECT_EQ(foo.field3_ref(), lazyFoo.field3_ref());
  EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
}

FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field1, LazyFoo, field1);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field2, LazyFoo, field2);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field3, LazyFoo, field3);
FBTHRIFT_DEFINE_MEMBER_ACCESSOR(get_field4, LazyFoo, field4);

TEST(Serialization, CheckDataMember) {
  auto foo = gen<LazyFoo>();
  auto s = CompactSerializer::serialize<std::string>(foo);
  auto lazyFoo = CompactSerializer::deserialize<LazyFoo>(s);

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

TEST(Serialization, CppRef) {
  {
    LazyCppRef foo;
    foo.field1_ref() = std::make_unique<std::vector<int32_t>>(10, 10);
    foo.field2_ref() = std::make_shared<std::vector<int32_t>>(20, 20);
    foo.field3_ref() = std::make_shared<std::vector<int32_t>>(30, 30);
    auto s = CompactSerializer::serialize<std::string>(foo);
    auto bar = CompactSerializer::deserialize<LazyCppRef>(s);
    EXPECT_EQ(*bar.field1_ref(), std::vector<int32_t>(10, 10));
    EXPECT_EQ(*bar.field2_ref(), std::vector<int32_t>(20, 20));
    EXPECT_EQ(*bar.field3_ref(), std::vector<int32_t>(30, 30));
  }
  {
    LazyCppRef foo;
    auto s = CompactSerializer::serialize<std::string>(foo);
    auto bar = CompactSerializer::deserialize<LazyCppRef>(s);
    EXPECT_FALSE(bar.field1_ref());
    EXPECT_FALSE(bar.field2_ref());
    EXPECT_FALSE(bar.field3_ref());
  }
}

TEST(Serialization, Comparison) {
  {
    auto foo1 = gen<LazyFoo>();
    auto s = CompactSerializer::serialize<std::string>(foo1);
    auto foo2 = CompactSerializer::deserialize<LazyFoo>(s);

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
    auto s = CompactSerializer::serialize<std::string>(foo1);
    auto foo2 = CompactSerializer::deserialize<LazyFoo>(s);

    foo1.field4_ref()->clear();

    EXPECT_LT(foo1, foo2);
  }
}

} // namespace apache::thrift::test
