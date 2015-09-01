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

#include "thrift/test/gen-cpp2/EnumTest_types.h"
#include <thrift/lib/cpp/util/EnumUtils.h>

#include <gtest/gtest.h>

using apache::thrift::TEnumTraits;
using namespace apache::thrift::util;
using namespace cpp2;

TEST(EnumTestCpp2, test_enum) {
  // Check that all the enum values match what we expect
  EXPECT_TRUE(TEnumTraits<MyEnum1>::min() == MyEnum1::ME1_0);
  EXPECT_EQ(int(MyEnum1::ME1_0), 0);
  EXPECT_EQ(int(MyEnum1::ME1_1), 1);
  EXPECT_EQ(int(MyEnum1::ME1_2), 2);
  EXPECT_EQ(int(MyEnum1::ME1_3), 3);
  EXPECT_EQ(int(MyEnum1::ME1_5), 5);
  EXPECT_EQ(int(MyEnum1::ME1_6), 6);
  EXPECT_TRUE(TEnumTraits<MyEnum1>::max() == MyEnum1::ME1_6);

  EXPECT_TRUE(TEnumTraits<MyEnum2>::min() == MyEnum2::ME2_0);
  EXPECT_EQ(int(MyEnum2::ME2_0), 0);
  EXPECT_EQ(int(MyEnum2::ME2_1), 1);
  EXPECT_EQ(int(MyEnum2::ME2_2), 2);
  EXPECT_TRUE(TEnumTraits<MyEnum2>::max() == MyEnum2::ME2_2);

  EXPECT_TRUE(TEnumTraits<MyEnum3>::min() == MyEnum3::ME3_N2);
  EXPECT_EQ(int(MyEnum3::ME3_0), 0);
  EXPECT_EQ(int(MyEnum3::ME3_1), 1);
  EXPECT_EQ(int(MyEnum3::ME3_N2), -2);
  EXPECT_EQ(int(MyEnum3::ME3_N1), -1);
  EXPECT_EQ(int(MyEnum3::ME3_D0), 0);
  EXPECT_EQ(int(MyEnum3::ME3_D1), 1);
  EXPECT_EQ(int(MyEnum3::ME3_9), 9);
  EXPECT_EQ(int(MyEnum3::ME3_10), 10);
  EXPECT_TRUE(TEnumTraits<MyEnum3>::max() == MyEnum3::ME3_10);

  EXPECT_TRUE(TEnumTraits<MyEnum4>::min() == MyEnum4::ME4_A);
  EXPECT_EQ(int(MyEnum4::ME4_A), 0x7ffffffd);
  EXPECT_EQ(int(MyEnum4::ME4_B), 0x7ffffffe);
  EXPECT_EQ(int(MyEnum4::ME4_C), 0x7fffffff);
  EXPECT_TRUE(TEnumTraits<MyEnum4>::max() == MyEnum4::ME4_C);
}

TEST(EnumTestCpp2, test_enum_constant) {
  MyStruct ms;
  EXPECT_TRUE(ms.me2_2 == MyEnum2::ME2_2);
  EXPECT_TRUE(ms.me3_n2 == MyEnum3::ME3_N2);
  EXPECT_TRUE(ms.me3_d1 == MyEnum3::ME3_D1);
}

TEST(EnumTestCpp2, test_enum_names) {
  EXPECT_EQ(enumName(MyEnum3::ME3_1), std::string{"ME3_1"});
  EXPECT_EQ(enumName(MyEnum2::ME2_2), std::string{"ME2_2"});
  EXPECT_EQ(enumName(static_cast<MyEnum2>(-10)), (const char*)nullptr);
  EXPECT_EQ(enumName(static_cast<MyEnum2>(-10), "foo"), "foo");
}

TEST(EnumTestCpp2, test_enum_parse) {
  MyEnum2 e2;
  MyEnum3 e3;

  EXPECT_TRUE(tryParseEnum("ME2_2", &e2));
  EXPECT_EQ((int)MyEnum2::ME2_2, (int)e2);
  EXPECT_TRUE(tryParseEnum("ME3_N2", &e3));
  EXPECT_EQ((int)MyEnum3::ME3_N2, (int)e3);

  EXPECT_FALSE(tryParseEnum("FOO_ME2_0", &e2));
  EXPECT_FALSE(tryParseEnum("BAR_ME3_N2", &e3));
}
