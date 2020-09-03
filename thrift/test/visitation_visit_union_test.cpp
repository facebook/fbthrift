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
#include <thrift/test/gen-cpp2/UnionFieldRef_visit_union.h>
using namespace std;

namespace apache {
namespace thrift {
namespace test {
TEST(UnionFieldTest, basic) {
  Basic a;
  visit_union(a, [&](auto&&, auto&&) { FAIL(); });

  static const string str = "foo";
  a.str_ref() = str;
  visit_union(a, [](auto&& meta, auto&& v) {
    EXPECT_EQ(*meta.name_ref(), "str");
    EXPECT_EQ(meta.type_ref()->getType(), meta.type_ref()->t_primitive);
    EXPECT_EQ(*meta.id_ref(), 2);
    EXPECT_EQ(*meta.is_optional_ref(), false);
    if constexpr (std::is_same_v<decltype(v), string&>) {
      EXPECT_EQ(v, str);
    } else {
      FAIL();
    }
  });

  static const int64_t int64 = 42LL << 42;
  a.int64_ref() = int64;
  visit_union(a, [](auto&& meta, auto&& v) {
    EXPECT_EQ(*meta.name_ref(), "int64");
    EXPECT_EQ(meta.type_ref()->getType(), meta.type_ref()->t_primitive);
    EXPECT_EQ(*meta.id_ref(), 1);
    EXPECT_EQ(*meta.is_optional_ref(), false);
    EXPECT_EQ(typeid(v), typeid(int64_t));
    if constexpr (std::is_same_v<decltype(v), int64_t&>) {
      EXPECT_EQ(v, int64);
    } else {
      FAIL();
    }
  });

  static const vector<int32_t> list_i32 = {3, 1, 2};
  a.list_i32_ref() = list_i32;
  visit_union(a, [](auto&& meta, auto&& v) {
    EXPECT_EQ(*meta.name_ref(), "list_i32");
    EXPECT_EQ(meta.type_ref()->getType(), meta.type_ref()->t_list);
    EXPECT_EQ(*meta.id_ref(), 4);
    EXPECT_EQ(*meta.is_optional_ref(), false);
    if constexpr (std::is_same_v<decltype(v), vector<int32_t>&>) {
      EXPECT_EQ(v, list_i32);
    } else {
      FAIL();
    }
  });
}
} // namespace test
} // namespace thrift
} // namespace apache
