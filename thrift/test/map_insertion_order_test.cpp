/*
 * Copyright 2016 Facebook, Inc.
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

#include <iostream>
#include <gtest/gtest.h>

#include <fatal/test/random_data.h>
#include <thrift/lib/cpp2/util/MapCopy.h>
#include <folly/Foreach.h>

#include <map>
#include <vector>

TEST(MapTest, test_small_map) {
  std::vector<std::pair<int, std::string>> data = {{1, "foo"}};
  std::map<int, std::string> expect = {{1, "foo"}};

  std::map<int, std::string> map;
  auto const ret = apache::thrift::move_ordered_map_vec(map, data);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(expect, map);
}

TEST(MapTest, test_unbalanced_map) {
  std::vector<std::pair<int, std::string>> data = {{1, "foo"}, {2, "bar"}};
  std::map<int, std::string> expect = {{1, "foo"}, {2, "bar"}};

  std::map<int, std::string> map;
  auto const ret = apache::thrift::move_ordered_map_vec(map, data);

  EXPECT_EQ(ret, 2);
  EXPECT_EQ(expect, map);
}

void test_map_size(std::size_t size) {
  std::vector<std::pair<int, std::string>> data;
  std::map<int, std::string> expect;
  fatal::random_data r;

  FOR_EACH_RANGE(i, 0, size) {
    auto const str_size = r() % 10;
    auto str = r.string(str_size);
    data.emplace_back(i, str);
    expect.emplace(i, str);
  }

  std::map<int, std::string> map;
  auto const ret = apache::thrift::move_ordered_map_vec(map, data);

  EXPECT_EQ(ret, size);
  EXPECT_EQ(expect, map);
}

// corner cases near a power of 2 (test first & fail fast)
TEST(MapTest, test_14_map) { test_map_size(14); }
TEST(MapTest, test_15_map) { test_map_size(15); }
TEST(MapTest, test_16_map) { test_map_size(16); }
TEST(MapTest, test_17_map) { test_map_size(17); }

TEST(MapTest, test_var_map) {
  for(uint32_t i = 0; i < 1000; i++) {
    test_map_size(i);
  }
}
