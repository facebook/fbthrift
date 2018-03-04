/*
 * Copyright 2018-present Facebook, Inc.
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

#include <thrift/test/gen-cpp2/fatal_optionals_types.h>
#include <thrift/test/gen-cpp2/fatal_optionals_fatal.h>

#include <gtest/gtest.h>

class FatalOptionalsTest : public testing::Test {};

TEST_F(FatalOptionalsTest, isset) {
  using type = apache::thrift::test::has_fields;
  using meta = apache::thrift::reflect_struct<type>;

  {
    using member = meta::member::reqd;

    type obj;
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_TRUE(member::mark_set(obj, true));
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_FALSE(member::mark_set(obj, false));
    EXPECT_TRUE(member::is_set(obj));
  }

  {
    using member = meta::member::norm;

    type obj;
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_TRUE(member::mark_set(obj, true));
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_FALSE(member::mark_set(obj, false));
    EXPECT_TRUE(member::is_set(obj));
  }

  {
    using member = meta::member::optl;

    type obj;
    EXPECT_FALSE(meta::member::optl::is_set(obj));

    EXPECT_TRUE(member::mark_set(obj, true));
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_FALSE(member::mark_set(obj, false));
    EXPECT_FALSE(member::is_set(obj));
  }

  {
    using member = meta::member::optl;

    type obj;
    obj.optl = 3;
    EXPECT_TRUE(meta::member::optl::is_set(obj));

    EXPECT_TRUE(member::mark_set(obj, true));
    EXPECT_TRUE(member::is_set(obj));

    EXPECT_FALSE(member::mark_set(obj, false));
    EXPECT_FALSE(member::is_set(obj));
  }
}
