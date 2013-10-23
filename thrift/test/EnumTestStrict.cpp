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
#include "thrift/lib/cpp/protocol/TJSONProtocol.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/test/gen-cpp/EnumTestStrict_types.h"
#include "thrift/test/gen-cpp/EnumTestStrict_constants.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

using namespace apache::thrift::util;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::protocol::TJSONProtocol;

BOOST_AUTO_TEST_SUITE( EnumTestStrict )

BOOST_AUTO_TEST_CASE( test_enum_strict ) {
  // Check that all the enum values match what we expect
  BOOST_CHECK_EQUAL((int)MyEnum1::ME1_0, 0);
  BOOST_CHECK_EQUAL((int)MyEnum1::ME1_1, 1);
  BOOST_CHECK_EQUAL((int)MyEnum1::ME1_2, 2);
  BOOST_CHECK_EQUAL((int)MyEnum1::ME1_3, 3);
  BOOST_CHECK_EQUAL((int)MyEnum1::ME1_5, 5);
  BOOST_CHECK_EQUAL((int)MyEnum1::ME1_6, 6);

  BOOST_CHECK_EQUAL((int)MyEnum2::ME2_0, 0);
  BOOST_CHECK_EQUAL((int)MyEnum2::ME2_1, 1);
  BOOST_CHECK_EQUAL((int)MyEnum2::ME2_2, 2);

  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_0, 0);
  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_1, 1);
  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_N2, -2);
  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_N1, -1);
  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_D0, 0);
  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_D1, 1);
  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_9, 9);
  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_10, 10);

  BOOST_CHECK_EQUAL((int)MyEnum4::ME4_A, 0x7ffffffd);
  BOOST_CHECK_EQUAL((int)MyEnum4::ME4_B, 0x7ffffffe);
  BOOST_CHECK_EQUAL((int)MyEnum4::ME4_C, 0x7fffffff);

  BOOST_CHECK_EQUAL(
    (int)g_EnumTestStrict_constants.c_me4_a,
    (int)MyEnum4::ME4_A);
}

BOOST_AUTO_TEST_CASE( test_enum_strict_constant ) {
  MyStruct ms;
  MyStruct ms2;
  BOOST_CHECK_EQUAL((int)ms.me2_2,  (int)MyEnum2::ME2_2);
  BOOST_CHECK_EQUAL((int)ms.me3_n2, (int)MyEnum3::ME3_N2);
  BOOST_CHECK_EQUAL((int)ms.me3_d1, (int)MyEnum3::ME3_D1);
  BOOST_CHECK_EQUAL(true, ms == ms2);

  BOOST_CHECK_EQUAL(
    (int)_MyEnum2_VALUES_TO_NAMES.find(MyEnum2::ME2_2)->first,
    (int)MyEnum2::ME2_2);

  BOOST_CHECK_EQUAL(0,
    strcmp(
      _MyEnum2_VALUES_TO_NAMES.find(MyEnum2::ME2_2)->second,
      "MyEnum2::ME2_2"));

  BOOST_CHECK_EQUAL(
    (int)_MyEnum3_NAMES_TO_VALUES.find("MyEnum3::ME3_10")->second,
    (int)MyEnum3::ME3_10);

  ms2.me2_2 = MyEnum2::ME2_1;
  BOOST_CHECK_EQUAL(false, ms == ms2);

  ms2.__clear();
  BOOST_CHECK_EQUAL(true, ms == ms2);
}

BOOST_AUTO_TEST_CASE( test_enum_names ) {
  BOOST_CHECK_EQUAL(enumName(MyEnum3::ME3_1), "MyEnum3::ME3_1");
  BOOST_CHECK_EQUAL(enumName(MyEnum2::ME2_2), "MyEnum2::ME2_2");
}

BOOST_AUTO_TEST_CASE( test_enum_parse ) {
  MyEnum2 e2;
  MyEnum3 e3;

  BOOST_CHECK_EQUAL(true, tryParseEnum("MyEnum2::ME2_2", &e2));
  BOOST_CHECK_EQUAL(true, tryParseEnum("MyEnum3::ME3_N2", &e3));
  BOOST_CHECK_EQUAL((int)MyEnum2::ME2_2, (int)e2);
  BOOST_CHECK_EQUAL((int)MyEnum3::ME3_N2, (int)e3);

  BOOST_CHECK_EQUAL(false, tryParseEnum("FOO_ME2_0", &e2));
  BOOST_CHECK_EQUAL(false, tryParseEnum("BAR_ME3_N2", &e3));
}

BOOST_AUTO_TEST_CASE( test_enum_strict_transport ) {
  MyStruct ms;
  MyStruct ms2;

  ms2.me2_2  = MyEnum2::ME2_1;
  ms2.me3_n2 = MyEnum3::ME3_9;
  ms2.me3_d1 = MyEnum3::ME3_10;
  BOOST_CHECK_EQUAL(false, ms == ms2);

  std::shared_ptr<TMemoryBuffer> buffer(new TMemoryBuffer());
  std::shared_ptr<TJSONProtocol> proto(new TJSONProtocol(buffer));

  ms.write(proto.get());
  ms2.read(proto.get());
  BOOST_CHECK_EQUAL(true, ms == ms2);
}

BOOST_AUTO_TEST_SUITE_END()
