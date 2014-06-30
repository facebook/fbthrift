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
#define BOOST_TEST_MODULE EnumTest
#include <boost/test/unit_test.hpp>
#include "thrift/test/gen-cpp/EnumTest_types.h"
#include <thrift/lib/cpp/util/EnumUtils.h>

using apache::thrift::TEnumTraits;
using namespace apache::thrift::util;

BOOST_AUTO_TEST_SUITE( EnumTest )

BOOST_AUTO_TEST_CASE( test_enum ) {
  // Check that all the enum values match what we expect
  BOOST_CHECK_EQUAL(TEnumTraits<MyEnum1>::min(), 0);
  BOOST_CHECK_EQUAL(ME1_0, 0);
  BOOST_CHECK_EQUAL(ME1_1, 1);
  BOOST_CHECK_EQUAL(ME1_2, 2);
  BOOST_CHECK_EQUAL(ME1_3, 3);
  BOOST_CHECK_EQUAL(ME1_5, 5);
  BOOST_CHECK_EQUAL(ME1_6, 6);
  BOOST_CHECK_EQUAL(TEnumTraits<MyEnum1>::max(), 6);

  BOOST_CHECK_EQUAL(TEnumTraits<MyEnum2>::min(), 0);
  BOOST_CHECK_EQUAL(ME2_0, 0);
  BOOST_CHECK_EQUAL(ME2_1, 1);
  BOOST_CHECK_EQUAL(ME2_2, 2);
  BOOST_CHECK_EQUAL(TEnumTraits<MyEnum2>::max(), 2);

  BOOST_CHECK_EQUAL(TEnumTraits<MyEnum3>::min(), -2);
  BOOST_CHECK_EQUAL(ME3_0, 0);
  BOOST_CHECK_EQUAL(ME3_1, 1);
  BOOST_CHECK_EQUAL(ME3_N2, -2);
  BOOST_CHECK_EQUAL(ME3_N1, -1);
  BOOST_CHECK_EQUAL(ME3_D0, 0);
  BOOST_CHECK_EQUAL(ME3_D1, 1);
  BOOST_CHECK_EQUAL(ME3_9, 9);
  BOOST_CHECK_EQUAL(ME3_10, 10);
  BOOST_CHECK_EQUAL(TEnumTraits<MyEnum3>::max(), 10);

  BOOST_CHECK_EQUAL(TEnumTraits<MyEnum4>::min(), 0x7ffffffd);
  BOOST_CHECK_EQUAL(ME4_A, 0x7ffffffd);
  BOOST_CHECK_EQUAL(ME4_B, 0x7ffffffe);
  BOOST_CHECK_EQUAL(ME4_C, 0x7fffffff);
  BOOST_CHECK_EQUAL(TEnumTraits<MyEnum4>::max(), 0x7fffffff);
}

BOOST_AUTO_TEST_CASE( test_enum_constant ) {
  MyStruct ms;
  BOOST_CHECK_EQUAL(ms.me2_2, 2);
  BOOST_CHECK_EQUAL(ms.me3_n2, -2);
  BOOST_CHECK_EQUAL(ms.me3_d1, 1);
}

BOOST_AUTO_TEST_CASE( test_enum_names ) {
  BOOST_CHECK_EQUAL(enumName(ME3_1), "ME3_1");
  BOOST_CHECK_EQUAL(enumName(ME2_2), "ME2_2");
  BOOST_CHECK_EQUAL(enumName(static_cast<MyEnum2>(-10)), (const char*)nullptr);
  BOOST_CHECK_EQUAL(enumName(static_cast<MyEnum2>(-10), "foo"), "foo");
}

BOOST_AUTO_TEST_CASE( test_enum_parse ) {
  MyEnum2 e2;
  MyEnum3 e3;

  BOOST_CHECK_EQUAL(true, tryParseEnum("ME2_2", &e2));
  BOOST_CHECK_EQUAL((int)ME2_2, (int)e2);
  BOOST_CHECK_EQUAL(true, tryParseEnum("ME3_N2", &e3));
  BOOST_CHECK_EQUAL((int)ME3_N2, (int)e3);

  BOOST_CHECK_EQUAL(false, tryParseEnum("FOO_ME2_0", &e2));
  BOOST_CHECK_EQUAL(false, tryParseEnum("BAR_ME3_N2", &e3));
}

BOOST_AUTO_TEST_SUITE_END()
