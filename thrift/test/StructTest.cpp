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

#include <thrift/test/gen-cpp2/structs_types.h>

#include <gtest/gtest.h>

using namespace apache::thrift::test;
using namespace folly;

class StructTest : public testing::Test {};

TEST_F(StructTest, equal_to) {
  std::equal_to<Basic> op;

  {
    Basic a;
    Basic b;

    a.def_field = 3;
    b.def_field = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.__isset.def_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.__isset.def_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    Basic a;
    Basic b;

    a.opt_field = 3;
    b.opt_field = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.__isset.opt_field = true;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.__isset.opt_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.opt_field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    Basic a;
    Basic b;

    a.req_field = 3;
    b.req_field = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.req_field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
}

// currently, binary fields are handled specially, so they get their own tests
TEST_F(StructTest, equal_to_binary) {
  std::equal_to<BasicBinaries> op;

  {
    BasicBinaries a;
    BasicBinaries b;

    a.def_field = "hello";
    b.def_field = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.__isset.def_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.__isset.def_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    BasicBinaries a;
    BasicBinaries b;

    a.opt_field = "hello";
    b.opt_field = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.__isset.opt_field = true;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.__isset.opt_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.opt_field = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    BasicBinaries a;
    BasicBinaries b;

    a.req_field = "hello";
    b.req_field = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.req_field = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
}

// current equal-to codegen implements def/opt/req ref fields with a single
// code path, so no need to test the different cases separately
TEST_F(StructTest, equal_to_refs) {
  std::equal_to<BasicRefs> op;

  {
    BasicRefs a;
    BasicRefs b;

    a.def_field = nullptr;
    b.def_field = nullptr;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field = std::make_unique<HasInt>();
    a.def_field->field = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = std::make_unique<HasInt>();
    b.def_field->field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field->field = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
}
