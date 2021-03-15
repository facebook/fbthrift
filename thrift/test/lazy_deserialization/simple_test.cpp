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
#include <thrift/test/lazy_deserialization/gen-cpp2/simple_types.h>

namespace apache::thrift::test {

// Use unique_ptr since assignement for lazy field is not implemented
template <class Struct>
std::unique_ptr<Struct> gen() {
  auto obj = std::make_unique<Struct>();
  obj->field1_ref().emplace(10, '1');
  obj->field2_ref().emplace(20, "2");
  obj->field3_ref().emplace(30, 3.0);
  for (int i = 0; i < 40; ++i) {
    obj->field4_ref().ensure()[i] = 4.0;
  }
  return obj;
}

TEST(Serialization, FooToLazyFoo) {
  auto foo = gen<Foo>();
  auto s = apache::thrift::CompactSerializer::serialize<std::string>(*foo);

  LazyFoo lazyFoo;
  apache::thrift::CompactSerializer::deserialize(s, lazyFoo);

  EXPECT_EQ(foo->field1_ref(), lazyFoo.field1_ref());
  EXPECT_EQ(foo->field2_ref(), lazyFoo.field2_ref());
  EXPECT_EQ(foo->field3_ref(), lazyFoo.field3_ref());
  EXPECT_EQ(foo->field4_ref(), lazyFoo.field4_ref());
}

TEST(Serialization, LazyFooToFoo) {
  auto lazyFoo = gen<LazyFoo>();
  auto s = apache::thrift::CompactSerializer::serialize<std::string>(*lazyFoo);

  Foo foo;
  apache::thrift::CompactSerializer::deserialize(s, foo);

  EXPECT_EQ(foo.field1_ref(), lazyFoo->field1_ref());
  EXPECT_EQ(foo.field2_ref(), lazyFoo->field2_ref());
  EXPECT_EQ(foo.field3_ref(), lazyFoo->field3_ref());
  EXPECT_EQ(foo.field4_ref(), lazyFoo->field4_ref());
}
} // namespace apache::thrift::test
