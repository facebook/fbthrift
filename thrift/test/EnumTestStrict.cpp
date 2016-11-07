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

#include <memory>
#include <thrift/lib/cpp/protocol/TJSONProtocol.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/test/gen-cpp/EnumTestStrict_types.h>
#include <thrift/test/gen-cpp/EnumTestStrict_constants.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include <gtest/gtest.h>

using namespace std;
using namespace apache::thrift::util;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::protocol::TJSONProtocol;

TEST(EnumTestStrict, test_enum_strict) {
  // Check that all the enum values match what we expect
  EXPECT_EQ((int)MyEnum1::ME1_0, 0);
  EXPECT_EQ((int)MyEnum1::ME1_1, 1);
  EXPECT_EQ((int)MyEnum1::ME1_2, 2);
  EXPECT_EQ((int)MyEnum1::ME1_3, 3);
  EXPECT_EQ((int)MyEnum1::ME1_5, 5);
  EXPECT_EQ((int)MyEnum1::ME1_6, 6);

  EXPECT_EQ((int)MyEnum2::ME2_0, 0);
  EXPECT_EQ((int)MyEnum2::ME2_1, 1);
  EXPECT_EQ((int)MyEnum2::ME2_2, 2);

  EXPECT_EQ((int)MyEnum3::ME3_0, 0);
  EXPECT_EQ((int)MyEnum3::ME3_1, 1);
  EXPECT_EQ((int)MyEnum3::ME3_N2, -2);
  EXPECT_EQ((int)MyEnum3::ME3_N1, -1);
  EXPECT_EQ((int)MyEnum3::ME3_D0, 0);
  EXPECT_EQ((int)MyEnum3::ME3_D1, 1);
  EXPECT_EQ((int)MyEnum3::ME3_9, 9);
  EXPECT_EQ((int)MyEnum3::ME3_10, 10);

  EXPECT_EQ((int)MyEnum4::ME4_A, 0x7ffffffd);
  EXPECT_EQ((int)MyEnum4::ME4_B, 0x7ffffffe);
  EXPECT_EQ((int)MyEnum4::ME4_C, 0x7fffffff);

  EXPECT_EQ(
    (int)EnumTestStrict_constants::c_me4_a(),
    (int)MyEnum4::ME4_A);
}

TEST(EnumTestStrict, test_enum_strict_constant) {
  MyStruct ms;
  MyStruct ms2;
  EXPECT_EQ((int)ms.me2_2,  (int)MyEnum2::ME2_2);
  EXPECT_EQ((int)ms.me3_n2, (int)MyEnum3::ME3_N2);
  EXPECT_EQ((int)ms.me3_d1, (int)MyEnum3::ME3_D1);
  EXPECT_EQ(ms, ms2);

  EXPECT_EQ(
    (int)_MyEnum2_VALUES_TO_NAMES.find(MyEnum2::ME2_2)->first,
    (int)MyEnum2::ME2_2);

  EXPECT_EQ(0,
    strcmp(
      _MyEnum2_VALUES_TO_NAMES.find(MyEnum2::ME2_2)->second,
      "ME2_2"));

  EXPECT_EQ(
    (int)_MyEnum3_NAMES_TO_VALUES.find("ME3_10")->second,
    (int)MyEnum3::ME3_10);

  ms2.me2_2 = MyEnum2::ME2_1;
  EXPECT_NE(ms, ms2);

  ms2.__clear();
  EXPECT_EQ(ms, ms2);
}

TEST(EnumTestStrict, test_enum_names) {
  EXPECT_EQ(enumName(MyEnum3::ME3_1), std::string{"ME3_1"});
  EXPECT_EQ(enumName(MyEnum2::ME2_2), std::string{"ME2_2"});
}

TEST(EnumTestStrict, test_enum_parse) {
  MyEnum2 e2;
  MyEnum3 e3;

  EXPECT_TRUE(tryParseEnum("ME2_2", &e2));
  EXPECT_TRUE(tryParseEnum("ME3_N2", &e3));
  EXPECT_EQ((int)MyEnum2::ME2_2, (int)e2);
  EXPECT_EQ((int)MyEnum3::ME3_N2, (int)e3);

  EXPECT_FALSE(tryParseEnum("FOO_ME2_0", &e2));
  EXPECT_FALSE(tryParseEnum("BAR_ME3_N2", &e3));
}

TEST(EnumTestStrict, test_enum_strict_transport) {
  MyStruct ms;
  MyStruct ms2;

  ms2.me2_2  = MyEnum2::ME2_1;
  ms2.me3_n2 = MyEnum3::ME3_9;
  ms2.me3_d1 = MyEnum3::ME3_10;
  EXPECT_NE(ms, ms2);

  auto buffer = make_shared<TMemoryBuffer>();
  auto proto = make_shared<TJSONProtocol>(buffer);

  ms.write(proto.get());
  ms2.read(proto.get());
  EXPECT_EQ(ms, ms2);
}
