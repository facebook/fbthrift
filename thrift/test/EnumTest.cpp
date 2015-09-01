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

#include "thrift/test/gen-cpp/EnumTest_types.h"
#include <thrift/lib/cpp/util/EnumUtils.h>

#include <gtest/gtest.h>

using apache::thrift::TEnumTraits;
using namespace apache::thrift::util;

TEST(EnumTest, test_enum) {
  // Check that all the enum values match what we expect
  EXPECT_EQ(TEnumTraits<MyEnum1>::min(), 0);
  EXPECT_EQ(ME1_0, 0);
  EXPECT_EQ(ME1_1, 1);
  EXPECT_EQ(ME1_2, 2);
  EXPECT_EQ(ME1_3, 3);
  EXPECT_EQ(ME1_5, 5);
  EXPECT_EQ(ME1_6, 6);
  EXPECT_EQ(TEnumTraits<MyEnum1>::max(), 6);

  EXPECT_EQ(TEnumTraits<MyEnum2>::min(), 0);
  EXPECT_EQ(ME2_0, 0);
  EXPECT_EQ(ME2_1, 1);
  EXPECT_EQ(ME2_2, 2);
  EXPECT_EQ(TEnumTraits<MyEnum2>::max(), 2);

  EXPECT_EQ(TEnumTraits<MyEnum3>::min(), -2);
  EXPECT_EQ(ME3_0, 0);
  EXPECT_EQ(ME3_1, 1);
  EXPECT_EQ(ME3_N2, -2);
  EXPECT_EQ(ME3_N1, -1);
  EXPECT_EQ(ME3_D0, 0);
  EXPECT_EQ(ME3_D1, 1);
  EXPECT_EQ(ME3_9, 9);
  EXPECT_EQ(ME3_10, 10);
  EXPECT_EQ(TEnumTraits<MyEnum3>::max(), 10);

  EXPECT_EQ(TEnumTraits<MyEnum4>::min(), 0x7ffffffd);
  EXPECT_EQ(ME4_A, 0x7ffffffd);
  EXPECT_EQ(ME4_B, 0x7ffffffe);
  EXPECT_EQ(ME4_C, 0x7fffffff);
  EXPECT_EQ(TEnumTraits<MyEnum4>::max(), 0x7fffffff);
}

TEST(EnumTest, test_enum_constant) {
  MyStruct ms;
  EXPECT_EQ(ms.me2_2, 2);
  EXPECT_EQ(ms.me3_n2, -2);
  EXPECT_EQ(ms.me3_d1, 1);
}

TEST(EnumTest, test_enum_names) {
  EXPECT_EQ(enumName(ME3_1), std::string{"ME3_1"});
  EXPECT_EQ(enumName(ME2_2), std::string{"ME2_2"});
  EXPECT_EQ(enumName(static_cast<MyEnum2>(-10)), (const char*)nullptr);
  EXPECT_EQ(enumName(static_cast<MyEnum2>(-10), "foo"), "foo");
}

TEST(EnumTest, test_enum_parse) {
  MyEnum2 e2;
  MyEnum3 e3;

  EXPECT_EQ(true, tryParseEnum("ME2_2", &e2));
  EXPECT_EQ((int)ME2_2, (int)e2);
  EXPECT_EQ(true, tryParseEnum("ME3_N2", &e3));
  EXPECT_EQ((int)ME3_N2, (int)e3);

  EXPECT_FALSE(tryParseEnum("FOO_ME2_0", &e2));
  EXPECT_FALSE(tryParseEnum("BAR_ME3_N2", &e3));
}
