/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "thrift/test/gen-cpp/Recursive_types.h"
#include "thrift/test/gen-cpp2/Recursive_types.h"
#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <gtest/gtest.h>

using namespace apache::thrift::util;

TEST(Recursive, copy1) {
  cpp1::RecList list1;
  list1.item = 10;
  list1.next.reset(new cpp1::RecList);
  list1.next->item = 20;
  cpp1::RecList list2{list1};
  EXPECT_EQ(list2, list1);

  cpp1::CoRec c1;
  c1.other.reset(new cpp1::CoRec2);
  cpp1::CoRec c2{c1};
  EXPECT_EQ(c1, c2);
}

TEST(Recursive, copy2) {
  cpp2::RecList list1;
  list1.item = 10;
  list1.next.reset(new cpp2::RecList);
  list1.next->item = 20;
  cpp2::RecList list2{list1};
  EXPECT_EQ(list2, list1);

  cpp2::CoRec c1;
  c1.other.reset(new cpp2::CoRec2);
  cpp2::CoRec c2{c1};
  EXPECT_EQ(c1, c2);
}

TEST(Recursive, assign1) {
  cpp1::RecList list1, list2;
  list1.item = 11;
  list2.next.reset(new cpp1::RecList);
  list2.next->item = 22;
  list2 = list1;
  EXPECT_EQ(list1, list2);
}

TEST(Recursive, assign2) {
  cpp2::RecList list1, list2;
  list1.item = 11;
  list2.next.reset(new cpp2::RecList);
  list2.next->item = 22;
  list2 = list1;
  EXPECT_EQ(list1, list2);
}

TEST(Recursive, merge) {
  cpp1::RecList list1;
  cpp1::RecList list2;

  merge(list1, list2);
}

TEST(Recursive, Tree) {
  cpp1::RecTree tree;
  cpp1::RecTree child;
  tree.children.push_back(child);

  ThriftSerializerBinary<void> serializer;
  std::string serialized;
  serializer.serialize(tree, &serialized);

  cpp1::RecTree result;
  serializer.deserialize(serialized, &result);
  EXPECT_EQ(tree, result);
}

TEST(Recursive, list1) {
  cpp1::RecList l;
  std::unique_ptr<cpp1::RecList> l2(new cpp1::RecList);
  l.next = std::move(l2);

  ThriftSerializerBinary<void> serializer;
  std::string serialized;
  serializer.serialize(l, &serialized);

  cpp1::RecList result;
  serializer.deserialize(serialized, &result);
  EXPECT_TRUE(l.next != nullptr);
  EXPECT_TRUE(result.next != nullptr);
  EXPECT_TRUE(l.next->next == nullptr);
  EXPECT_TRUE(result.next->next == nullptr);
}

TEST(Recursive, list2) {
  cpp2::RecList l;
  std::unique_ptr<cpp2::RecList> l2(new cpp2::RecList);
  l.next = std::move(l2);

  auto serializer = apache::thrift::CompactSerializer();
  folly::IOBufQueue bufq;
  serializer.serialize(l, &bufq);

  cpp2::RecList result;
  serializer.deserialize(bufq.front(), result);
  EXPECT_TRUE(l.next != nullptr);
  EXPECT_TRUE(result.next != nullptr);
  EXPECT_TRUE(l.next->next == nullptr);
  EXPECT_TRUE(result.next->next == nullptr);
}

TEST(Recursive, CoRec1) {
  cpp1::CoRec c;
  std::unique_ptr<cpp1::CoRec2> r(new cpp1::CoRec2);
  c.other = std::move(r);

  ThriftSerializerBinary<void> serializer;
  std::string serialized;
  serializer.serialize(c, &serialized);

  cpp1::CoRec result;
  serializer.deserialize(serialized, &result);
  EXPECT_TRUE(result.other != nullptr);
  EXPECT_TRUE(result.other->other.other == nullptr);
}

TEST(Recursive, CoRec2) {
  cpp2::CoRec c;
  std::unique_ptr<cpp2::CoRec2> r(new cpp2::CoRec2);
  c.other = std::move(r);

  auto serializer = apache::thrift::CompactSerializer();
  folly::IOBufQueue bufq;
  serializer.serialize(c, &bufq);

  cpp2::CoRec result;
  serializer.deserialize(bufq.front(), result);
  EXPECT_TRUE(result.other != nullptr);
  EXPECT_TRUE(result.other->other.other == nullptr);
}

/** TODO(7372649): Fails due to cpp.ref = "true" roundtrip bug
TEST(Recursive, Roundtrip1) {
  cpp1::MyStruct strct;
  std::unique_ptr<cpp1::MyField> field(new cpp1::MyField);
  strct.field = std::move(field);

  ThriftSerializerBinary<void> serializer;
  std::string serialized;
  serializer.serialize(strct, &serialized);

  cpp1::MyStruct result;
  serializer.deserialize(serialized, &result);
  EXPECT_TRUE(result.field != nullptr);
}
*/

TEST(Recursive, Roundtrip2) {
  cpp2::MyStruct strct;
  std::unique_ptr<cpp2::MyField> field(new cpp2::MyField);
  strct.field = std::move(field);

  auto serializer = apache::thrift::CompactSerializer();
  folly::IOBufQueue bufq;
  serializer.serialize(strct, &bufq);

  cpp2::MyStruct result;
  serializer.deserialize(bufq.front(), result);
  EXPECT_TRUE(result.field != nullptr);
}

TEST(Recursive, CoRecJson) {
  cpp1::CoRec c;
  std::unique_ptr<cpp1::CoRec2> r(new cpp1::CoRec2);
  c.other = std::move(r);

  ThriftSerializerSimpleJson<void> serializer;
  std::string serialized;
  serializer.serialize(c, &serialized);

  cpp1::RecList result;
  serializer.deserialize(serialized, &result);
  EXPECT_TRUE(c.other != nullptr);
  EXPECT_TRUE(c.other->other.other == nullptr);
}

TEST(Recursive, ReadFromJson) {
  cpp1::CoRec c;
  c.readFromJson("{\"other\": { \"other\": {} } }");
}
