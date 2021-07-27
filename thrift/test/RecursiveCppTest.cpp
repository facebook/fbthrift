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
#include <thrift/test/gen-cpp2/Recursive_types.h>

using namespace apache::thrift::test;

TEST(Recursive, copy) {
  RecList list1;
  *list1.item_ref() = 10;
  list1.next.reset(new RecList);
  *list1.next->item_ref() = 20;
  RecList list2{list1};
  EXPECT_EQ(list2, list1);

  CoRec c1;
  c1.other.reset(new CoRec2);
  CoRec c2{c1};
  EXPECT_EQ(c1, c2);
}

TEST(Recursive, assign) {
  RecList list1, list2;
  *list1.item_ref() = 11;
  list2.next.reset(new RecList);
  *list2.next->item_ref() = 22;
  list2 = list1;
  EXPECT_EQ(list1, list2);
}

TEST(Recursive, Tree) {
  RecTree tree;
  RecTree child;
  tree.children_ref()->push_back(child);

  auto serializer = apache::thrift::CompactSerializer();
  folly::IOBufQueue bufq;
  serializer.serialize(tree, &bufq);

  RecTree result;
  serializer.deserialize(bufq.front(), result);
  EXPECT_EQ(tree, result);
}

TEST(Recursive, list) {
  RecList l;
  std::unique_ptr<RecList> l2(new RecList);
  l.next = std::move(l2);

  auto serializer = apache::thrift::CompactSerializer();
  folly::IOBufQueue bufq;
  serializer.serialize(l, &bufq);

  RecList result;
  serializer.deserialize(bufq.front(), result);
  EXPECT_TRUE(l.next != nullptr);
  EXPECT_TRUE(result.next != nullptr);
  EXPECT_TRUE(l.next->next == nullptr);
  EXPECT_TRUE(result.next->next == nullptr);
}

TEST(Recursive, CoRec) {
  CoRec c;
  std::unique_ptr<CoRec2> r(new CoRec2);
  c.other = std::move(r);

  auto serializer = apache::thrift::CompactSerializer();
  folly::IOBufQueue bufq;
  serializer.serialize(c, &bufq);

  CoRec result;
  serializer.deserialize(bufq.front(), result);
  EXPECT_TRUE(result.other != nullptr);
  EXPECT_TRUE(result.other->other_ref()->other == nullptr);
}

TEST(Recursive, Roundtrip) {
  MyStruct strct;
  std::unique_ptr<MyField> field(new MyField);
  strct.field = std::move(field);

  auto serializer = apache::thrift::CompactSerializer();
  folly::IOBufQueue bufq;
  serializer.serialize(strct, &bufq);

  MyStruct result;
  serializer.deserialize(bufq.front(), result);
  EXPECT_TRUE(result.field != nullptr);
}

TEST(Recursive, CoRecJson) {
  CoRec c;
  std::unique_ptr<CoRec2> r(new CoRec2);
  c.other = std::move(r);

  auto serializer = apache::thrift::SimpleJSONSerializer();
  folly::IOBufQueue bufq;
  serializer.serialize(c, &bufq);

  RecList result;
  serializer.deserialize(bufq.front(), result);
  EXPECT_TRUE(c.other != nullptr);
  EXPECT_TRUE(c.other->other_ref()->other == nullptr);
}

TEST(Recursive, StructUsingAnnotation) {
  StructUsingAnnotation s;

  s.field_ref() = MyField();
  s.field_ref()->some_val_ref() = 5;
  MyField m;
  m.some_val_ref() = 5;
  EXPECT_EQ(s.field_ref().value(), m);

  StructUsingAnnotation t = s;
  EXPECT_EQ(t.field_ref()->some_val_ref().value(), 5);

  StructUsingAnnotation x = std::move(t);
  EXPECT_EQ(x.field_ref()->some_val_ref().value(), 5);

  auto serializer = apache::thrift::CompactSerializer();
  folly::IOBufQueue bufq;
  serializer.serialize(x, &bufq);

  StructUsingAnnotation result;
  serializer.deserialize(bufq.front(), result);
  EXPECT_TRUE(result.field_ref().has_value());
  EXPECT_EQ(result.field_ref()->some_val_ref().value(), 5);
}
