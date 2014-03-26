/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <boost/test/unit_test.hpp>

#include "thrift/test/gen-cpp/ThriftTest_types.h"
#include "thrift/test/gen-cpp/OptionalRequiredTest_types.h"

using namespace std;

BOOST_AUTO_TEST_CASE(test_swap_xtruct2) {
  using namespace thrift::test;

  Xtruct2 a;
  Xtruct2 b;

  a.byte_thing = 12;
  a.struct_thing.string_thing = "foobar";
  a.struct_thing.byte_thing = 42;
  a.struct_thing.i32_thing = 0;
  a.struct_thing.i64_thing = 0x1234567887654321LL;
  a.i32_thing = 0x7fffffff;

  b.byte_thing = 0x7f;
  b.struct_thing.string_thing = "abcdefghijklmnopqrstuvwxyz";
  b.struct_thing.byte_thing = -1;
  b.struct_thing.i32_thing = 99;
  b.struct_thing.i64_thing = 10101;
  b.i32_thing = 0xdeadbeef;

  swap(a, b);

  BOOST_CHECK_EQUAL(b.byte_thing, 12);
  BOOST_CHECK_EQUAL(b.struct_thing.string_thing, "foobar");
  BOOST_CHECK_EQUAL(b.struct_thing.byte_thing, 42);
  BOOST_CHECK_EQUAL(b.struct_thing.i32_thing, 0);
  BOOST_CHECK_EQUAL(b.struct_thing.i64_thing, 0x1234567887654321LL);
  BOOST_CHECK_EQUAL(b.i32_thing, 0x7fffffff);

  BOOST_CHECK_EQUAL(a.byte_thing, 0x7f);
  BOOST_CHECK_EQUAL(a.struct_thing.string_thing, "abcdefghijklmnopqrstuvwxyz");
  BOOST_CHECK_EQUAL(a.struct_thing.byte_thing, -1);
  BOOST_CHECK_EQUAL(a.struct_thing.i32_thing, 99);
  BOOST_CHECK_EQUAL(a.struct_thing.i64_thing, 10101);
  BOOST_CHECK_EQUAL(a.i32_thing, 0xdeadbeef);
}

void check_simple(const thrift::test::Simple &s1,
                  const thrift::test::Simple &s2) {
  // Explicitly compare the fields, since the generated == operator
  // ignores optional fields that are marked as not set.  Also,
  // this allows us to use the BOOST_CHECK_EQUAL, so the values are printed
  // when they don't match.
  BOOST_CHECK_EQUAL(s1.im_default, s2.im_default);
  BOOST_CHECK_EQUAL(s1.im_required, s2.im_required);
  BOOST_CHECK_EQUAL(s1.im_optional, s2.im_optional);
  BOOST_CHECK_EQUAL(s1.__isset.im_default, s2.__isset.im_default);
  BOOST_CHECK_EQUAL(s1.__isset.im_optional, s2.__isset.im_optional);
}

BOOST_AUTO_TEST_CASE(test_swap_optional) {
  using thrift::test::Complex;
  using thrift::test::Simple;

  Complex comp1;
  Complex comp2;

  Simple simple1;
  simple1.im_default = 1;
  simple1.im_required = 1;
  simple1.im_optional = 1;
  simple1.__isset.im_default = true;
  simple1.__isset.im_optional = true;

  Simple simple2;
  simple2.im_default = 2;
  simple2.im_required = 2;
  simple2.im_optional = 2;
  simple2.__isset.im_default = false;
  simple2.__isset.im_optional = true;

  Simple simple3;
  simple3.im_default = 3;
  simple3.im_required = 3;
  simple3.im_optional = 3;
  simple3.__isset.im_default = true;
  simple3.__isset.im_optional = false;

  Simple simple4;
  simple4.im_default = 4;
  simple4.im_required = 4;
  simple4.im_optional = 4;
  simple4.__isset.im_default = false;
  simple4.__isset.im_optional = false;

  comp1.cp_default = 5;
  comp1.__isset.cp_default = true;
  comp1.cp_required = 0x7fff;
  comp1.cp_optional = 50;
  comp1.__isset.cp_optional = true;
  comp1.the_map.insert(make_pair(1, simple1));
  comp1.the_map.insert(make_pair(99, simple2));
  comp1.the_map.insert(make_pair(-7, simple3));
  comp1.req_simp = simple4;
  comp1.opt_simp = simple1;
  comp1.__isset.opt_simp = false;

  comp2.cp_default = -7;
  comp2.__isset.cp_default = false;
  comp2.cp_required = 0;
  comp2.cp_optional = 0;
  comp2.__isset.cp_optional = false;
  comp2.the_map.insert(make_pair(6, simple2));
  comp2.req_simp = simple1;
  comp2.opt_simp = simple3;
  comp2.__isset.opt_simp = true;

  swap(comp1, comp2);

  BOOST_CHECK_EQUAL(comp1.cp_default, -7);
  BOOST_CHECK_EQUAL(comp1.__isset.cp_default, false);
  BOOST_CHECK_EQUAL(comp1.cp_required, 0);
  BOOST_CHECK_EQUAL(comp1.cp_optional, 0);
  BOOST_CHECK_EQUAL(comp1.__isset.cp_optional, false);
  BOOST_CHECK_EQUAL(comp1.the_map.size(), 1);
  check_simple(comp1.the_map[6], simple2);
  check_simple(comp1.req_simp, simple1);
  check_simple(comp1.opt_simp, simple3);
  BOOST_CHECK_EQUAL(comp1.__isset.opt_simp, true);

  BOOST_CHECK_EQUAL(comp2.cp_default, 5);
  BOOST_CHECK_EQUAL(comp2.__isset.cp_default, true);
  BOOST_CHECK_EQUAL(comp2.cp_required, 0x7fff);
  BOOST_CHECK_EQUAL(comp2.cp_optional, 50);
  BOOST_CHECK_EQUAL(comp2.__isset.cp_optional, true);
  BOOST_CHECK_EQUAL(comp2.the_map.size(), 3);
  check_simple(comp2.the_map[1], simple1);
  check_simple(comp2.the_map[99], simple2);
  check_simple(comp2.the_map[-7], simple3);
  check_simple(comp2.req_simp, simple4);
  check_simple(comp2.opt_simp, simple1);
  BOOST_CHECK_EQUAL(comp2.__isset.opt_simp, false);
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  boost::unit_test::framework::master_test_suite().p_name.value =
    "SwapTest";

  if (argc != 1) {
    fprintf(stderr, "unexpected arguments: %s\n", argv[1]);
    exit(1);
  }

  return nullptr;
}
