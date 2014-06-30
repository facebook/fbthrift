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

#define BOOST_TEST_MODULE EnumTest
#include <boost/test/unit_test.hpp>
#include "thrift/test/gen-cpp2/EnumTest_types.h"
#include <thrift/lib/cpp/util/EnumUtils.h>

using apache::thrift::TEnumTraits;
using namespace apache::thrift::util;
using namespace cpp2;

BOOST_AUTO_TEST_SUITE( EnumTest )

BOOST_AUTO_TEST_CASE( test_enum ) {
  // Check that all the enum values match what we expect
  BOOST_CHECK(TEnumTraits<MyEnum1>::min() == MyEnum1::ME1_0);
  BOOST_CHECK_EQUAL(int(MyEnum1::ME1_0), 0);
  BOOST_CHECK_EQUAL(int(MyEnum1::ME1_1), 1);
  BOOST_CHECK_EQUAL(int(MyEnum1::ME1_2), 2);
  BOOST_CHECK_EQUAL(int(MyEnum1::ME1_3), 3);
  BOOST_CHECK_EQUAL(int(MyEnum1::ME1_5), 5);
  BOOST_CHECK_EQUAL(int(MyEnum1::ME1_6), 6);
  BOOST_CHECK(TEnumTraits<MyEnum1>::max() == MyEnum1::ME1_6);

  BOOST_CHECK(TEnumTraits<MyEnum2>::min() == MyEnum2::ME2_0);
  BOOST_CHECK_EQUAL(int(MyEnum2::ME2_0), 0);
  BOOST_CHECK_EQUAL(int(MyEnum2::ME2_1), 1);
  BOOST_CHECK_EQUAL(int(MyEnum2::ME2_2), 2);
  BOOST_CHECK(TEnumTraits<MyEnum2>::max() == MyEnum2::ME2_2);

  BOOST_CHECK(TEnumTraits<MyEnum3>::min() == MyEnum3::ME3_N2);
  BOOST_CHECK_EQUAL(int(MyEnum3::ME3_0), 0);
  BOOST_CHECK_EQUAL(int(MyEnum3::ME3_1), 1);
  BOOST_CHECK_EQUAL(int(MyEnum3::ME3_N2), -2);
  BOOST_CHECK_EQUAL(int(MyEnum3::ME3_N1), -1);
  BOOST_CHECK_EQUAL(int(MyEnum3::ME3_D0), 0);
  BOOST_CHECK_EQUAL(int(MyEnum3::ME3_D1), 1);
  BOOST_CHECK_EQUAL(int(MyEnum3::ME3_9), 9);
  BOOST_CHECK_EQUAL(int(MyEnum3::ME3_10), 10);
  BOOST_CHECK(TEnumTraits<MyEnum3>::max() == MyEnum3::ME3_10);

  BOOST_CHECK(TEnumTraits<MyEnum4>::min() == MyEnum4::ME4_A);
  BOOST_CHECK_EQUAL(int(MyEnum4::ME4_A), 0x7ffffffd);
  BOOST_CHECK_EQUAL(int(MyEnum4::ME4_B), 0x7ffffffe);
  BOOST_CHECK_EQUAL(int(MyEnum4::ME4_C), 0x7fffffff);
  BOOST_CHECK(TEnumTraits<MyEnum4>::max() == MyEnum4::ME4_C);
}

BOOST_AUTO_TEST_CASE( test_enum_constant ) {
  MyStruct ms;
  BOOST_CHECK(ms.me2_2 == MyEnum2::ME2_2);
  BOOST_CHECK(ms.me3_n2 == MyEnum3::ME3_N2);
  BOOST_CHECK(ms.me3_d1 == MyEnum3::ME3_D1);
}

BOOST_AUTO_TEST_CASE( test_enum_names ) {
  BOOST_CHECK_EQUAL(enumName(MyEnum3::ME3_1), "ME3_1");
  BOOST_CHECK_EQUAL(enumName(MyEnum2::ME2_2), "ME2_2");
  BOOST_CHECK_EQUAL(enumName(static_cast<MyEnum2>(-10)), (const char*)nullptr);
  BOOST_CHECK_EQUAL(enumName(static_cast<MyEnum2>(-10), "foo"), "foo");
}

BOOST_AUTO_TEST_CASE( test_enum_parse ) {
  MyEnum2 e2;
  MyEnum3 e3;

  BOOST_CHECK_EQUAL(true, tryParseEnum("ME2_2", &e2));
  BOOST_CHECK_EQUAL((int)MyEnum2::ME2_2, (int)e2);
  BOOST_CHECK_EQUAL(true, tryParseEnum("ME3_N2", &e3));
  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_N2, (int)e3);

  BOOST_CHECK_EQUAL(false, tryParseEnum("FOO_ME2_0", &e2));
  BOOST_CHECK_EQUAL(false, tryParseEnum("BAR_ME3_N2", &e3));
}

BOOST_AUTO_TEST_SUITE_END()
