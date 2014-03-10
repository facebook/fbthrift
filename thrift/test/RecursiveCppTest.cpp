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
#include "thrift/lib/cpp/util/ThriftSerializer.h"
#include <gtest/gtest.h>

using namespace apache::thrift::util;

TEST(Recursive, merge) {
  RecList list1;
  RecList list2;

  merge(list1, list2);
  EXPECT_THROW(merge(list1.next, list2.next), apache::thrift::TLibraryException);
}

TEST(Recursive, Tree) {
  RecTree tree;
  RecTree child;
  tree.children.push_back(child);

  ThriftSerializerBinary<void> serializer;
  std::string serialized;
  serializer.serialize(tree, &serialized);

  RecTree result;
  serializer.deserialize(serialized, &result);
  EXPECT_EQ(tree, result);
}

TEST(Recursive, list) {
  RecList l;
  std::unique_ptr<RecList> l2(new RecList);
  l.next = std::move(l2);

  ThriftSerializerBinary<void> serializer;
  std::string serialized;
  serializer.serialize(l, &serialized);

  RecList result;
  serializer.deserialize(serialized, &result);
  EXPECT_TRUE(result.next != nullptr);
  EXPECT_TRUE(result.next->next == nullptr);
}

TEST(Recursive, CoRec) {
  CoRec c;
  std::unique_ptr<CoRec2> r(new CoRec2);
  c.other = std::move(r);

  ThriftSerializerBinary<void> serializer;
  std::string serialized;
  serializer.serialize(c, &serialized);

  RecList result;
  serializer.deserialize(serialized, &result);
  EXPECT_TRUE(c.other != nullptr);
  EXPECT_TRUE(c.other->other.other == nullptr);
}
