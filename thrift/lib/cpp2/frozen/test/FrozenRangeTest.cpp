/*
 * Copyright 2014-present Facebook, Inc.
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
#include <thrift/lib/cpp2/frozen/FrozenUtil.h>

namespace apache {
namespace thrift {
using namespace frozen;
using namespace util;

TEST(FrozenRange, ArrayLike) {
  std::vector<int> v{10, 11, 12, 13};
  auto fv = freeze(v);
  EXPECT_EQ(fv.size(), 4);
  EXPECT_FALSE(fv.empty());
  EXPECT_EQ(fv[2], 12);
}

TEST(FrozenRange, Iterators) {
  std::vector<int> v{10, 11, 12, 13};
  auto fv = freeze(v);
  auto it = fv.begin();
  EXPECT_EQ(*(it + 2), 12);
  EXPECT_EQ(*it, 10);
  EXPECT_EQ(*it++, 10);
  EXPECT_EQ(*it++, 11);
  EXPECT_EQ(*it, 12);
  EXPECT_EQ(*(it - 2), 10);
  EXPECT_EQ(*++it, 13);
  it = fv.end();
  EXPECT_EQ(*--it, 13);
  EXPECT_EQ(*it--, 13);
  EXPECT_EQ(*it--, 12);
  EXPECT_EQ(*--it, 10);
}

TEST(FrozenRange, Zeros) {
  std::vector<int> v(1000, 0);
  // If all the values are zero-sized, we only need space to store the count.
  EXPECT_EQ(frozenSize(v), 2);
  // only a single byte for 200 empty strings
  EXPECT_EQ(frozenSize(std::vector<std::string>(200)), 1);
  auto fv = freeze(v);
  EXPECT_EQ(fv.size(), 1000);
  auto it = fv.begin();
  EXPECT_EQ(*it++, 0);
  EXPECT_EQ(*it++, 0);
  size_t n = 0;
  for (auto& item : fv) {
    ++n; // ensure iteration still works despite zero-byte items
  }
  EXPECT_EQ(n, 1000);
}

TEST(FrozenRange, VectorVectorInt) {
  std::vector<std::vector<int>> vvi{{2, 3, 5, 7}, {11, 13, 17, 19}};
  auto fvvi = freeze(vvi);
  auto tvvi = fvvi.thaw();
  EXPECT_EQ(tvvi, vvi);
}
}
}
