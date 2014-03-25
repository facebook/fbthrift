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
#include <gtest/gtest.h>
#include "thrift/lib/cpp2/frozen/VectorAssociative.h"
#include "thrift/lib/cpp2/frozen/FrozenTestUtil.h"

using namespace apache::thrift;
using namespace frozen;
using namespace util;

TEST(FrozenDummies, VectorAsMap) {
  VectorAsMap<int, int> dm;
  dm.insert({1, 2});
  dm.insert(dm.end(), {3, 4});
  auto fdm = freeze(dm);
  EXPECT_EQ(2, fdm.at(1));
  EXPECT_EQ(4, fdm.at(3));
}

TEST(FrozenDummies, VectorAsHashMap) {
  VectorAsHashMap<int, int> dm;
  dm.insert({1, 2});
  dm.insert(dm.end(), {3, 4});
  auto fdm = freeze(dm);
  EXPECT_EQ(2, fdm.at(1));
  EXPECT_EQ(4, fdm.at(3));
}

TEST(FrozenDummies, VectorAsSet) {
  VectorAsSet<int> dm;
  dm.insert(3);
  dm.insert(dm.end(), 7);
  auto fdm = freeze(dm);
  EXPECT_EQ(1, fdm.count(3));
  EXPECT_EQ(1, fdm.count(7));
  EXPECT_EQ(0, fdm.count(4));
}

TEST(FrozenDummies, VectorAsHashSet) {
  VectorAsHashSet<int> dm;
  dm.insert(3);
  dm.insert(dm.end(), 7);
  auto fdm = freeze(dm);
  EXPECT_EQ(1, fdm.count(3));
  EXPECT_EQ(1, fdm.count(7));
  EXPECT_EQ(0, fdm.count(4));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
