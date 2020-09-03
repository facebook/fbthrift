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

#include <thrift/test/gen-cpp2/OptionalRequiredTest_types.h>
#include <thrift/test/gen-cpp2/ThriftTest_types.h>

#include <folly/portability/GTest.h>

using namespace std;

TEST(SwapTest, test_swap_xtruct2) {
  using namespace thrift::test;

  Xtruct2 a;
  Xtruct2 b;

  *a.byte_thing_ref() = 12;
  *a.struct_thing_ref()->string_thing_ref() = "foobar";
  *a.struct_thing_ref()->byte_thing_ref() = 42;
  *a.struct_thing_ref()->i32_thing_ref() = 0;
  *a.struct_thing_ref()->i64_thing_ref() = 0x1234567887654321LL;
  *a.i32_thing_ref() = 0x7fffffff;

  *b.byte_thing_ref() = 0x7f;
  *b.struct_thing_ref()->string_thing_ref() = "abcdefghijklmnopqrstuvwxyz";
  *b.struct_thing_ref()->byte_thing_ref() = -1;
  *b.struct_thing_ref()->i32_thing_ref() = 99;
  *b.struct_thing_ref()->i64_thing_ref() = 10101;
  *b.i32_thing_ref() = 0xdeadbeef;

  swap(a, b);

  EXPECT_EQ(*b.byte_thing_ref(), 12);
  EXPECT_EQ(*b.struct_thing_ref()->string_thing_ref(), "foobar");
  EXPECT_EQ(*b.struct_thing_ref()->byte_thing_ref(), 42);
  EXPECT_EQ(*b.struct_thing_ref()->i32_thing_ref(), 0);
  EXPECT_EQ(*b.struct_thing_ref()->i64_thing_ref(), 0x1234567887654321LL);
  EXPECT_EQ(*b.i32_thing_ref(), 0x7fffffff);

  EXPECT_EQ(*a.byte_thing_ref(), 0x7f);
  EXPECT_EQ(
      *a.struct_thing_ref()->string_thing_ref(), "abcdefghijklmnopqrstuvwxyz");
  EXPECT_EQ(*a.struct_thing_ref()->byte_thing_ref(), -1);
  EXPECT_EQ(*a.struct_thing_ref()->i32_thing_ref(), 99);
  EXPECT_EQ(*a.struct_thing_ref()->i64_thing_ref(), 10101);
  EXPECT_EQ(*a.i32_thing_ref(), 0xdeadbeef);
}

void check_simple(
    const apache::thrift::test::Simple& s1,
    const apache::thrift::test::Simple& s2) {
  // Explicitly compare the fields, since the generated == operator
  // ignores optional fields that are marked as not set.  Also,
  // this allows us to use the EXPECT_EQ, so the values are printed
  // when they don't match.
  EXPECT_EQ(*s1.im_default_ref(), *s2.im_default_ref());
  EXPECT_EQ(s1.im_required, s2.im_required);
  EXPECT_EQ(s1.im_optional_ref(), s2.im_optional_ref());
  EXPECT_EQ(s1.__isset.im_default, s2.__isset.im_default);
}

TEST(SwapTest, test_swap_optional) {
  using apache::thrift::test::Complex;
  using apache::thrift::test::Simple;

  Complex comp1;
  Complex comp2;

  Simple simple1;
  *simple1.im_default_ref() = 1;
  simple1.im_required = 1;
  simple1.im_optional_ref() = 1;
  simple1.__isset.im_default = true;

  Simple simple2;
  *simple2.im_default_ref() = 2;
  simple2.im_required = 2;
  simple2.im_optional_ref() = 2;
  simple2.__isset.im_default = false;

  Simple simple3;
  *simple3.im_default_ref() = 3;
  simple3.im_required = 3;
  simple3.im_optional_ref() = 3;
  simple3.__isset.im_default = true;

  Simple simple4;
  *simple4.im_default_ref() = 4;
  simple4.im_required = 4;
  simple4.im_optional_ref() = 4;
  simple4.__isset.im_default = false;

  *comp1.cp_default_ref() = 5;
  comp1.__isset.cp_default = true;
  comp1.cp_required = 0x7fff;
  comp1.cp_optional_ref() = 50;
  comp1.the_map_ref()->insert(make_pair(1, simple1));
  comp1.the_map_ref()->insert(make_pair(99, simple2));
  comp1.the_map_ref()->insert(make_pair(-7, simple3));
  comp1.req_simp = simple4;
  comp1.opt_simp_ref().reset();

  *comp2.cp_default_ref() = -7;
  comp2.__isset.cp_default = false;
  comp2.cp_required = 0;
  comp2.cp_optional_ref().reset();
  comp2.the_map_ref()->insert(make_pair(6, simple2));
  comp2.req_simp = simple1;
  comp2.opt_simp_ref() = simple3;

  swap(comp1, comp2);

  EXPECT_EQ(*comp1.cp_default_ref(), -7);
  EXPECT_EQ(comp1.__isset.cp_default, false);
  EXPECT_EQ(comp1.cp_required, 0);
  EXPECT_FALSE(comp1.cp_optional_ref().has_value());
  EXPECT_EQ(comp1.the_map_ref()->size(), 1);
  check_simple(comp1.the_map_ref()[6], simple2);
  check_simple(comp1.req_simp, simple1);
  check_simple(*comp1.opt_simp_ref(), simple3);

  EXPECT_EQ(*comp2.cp_default_ref(), 5);
  EXPECT_EQ(comp2.__isset.cp_default, true);
  EXPECT_EQ(comp2.cp_required, 0x7fff);
  EXPECT_EQ(*comp2.cp_optional_ref(), 50);
  EXPECT_EQ(comp2.the_map_ref()->size(), 3);
  check_simple(comp2.the_map_ref()[1], simple1);
  check_simple(comp2.the_map_ref()[99], simple2);
  check_simple(comp2.the_map_ref()[-7], simple3);
  check_simple(comp2.req_simp, simple4);
  EXPECT_EQ(comp2.__isset.opt_simp, false);
}
