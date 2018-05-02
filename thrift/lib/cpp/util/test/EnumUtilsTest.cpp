/*
 * Copyright 2004-present Facebook, Inc.
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

#include <gtest/gtest.h>

#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp/util/test/gen-cpp/enum_strict_types.h>

using namespace apache::thrift::util;

TEST(EnumUtilsTest, ShortEnumName) {
  EXPECT_STREQ(enumName((EnumStrict)0), "UNKNOWN");
  EXPECT_STREQ(enumName((EnumStrict)1), "VALUE");
  EXPECT_STREQ(enumName((EnumStrict)2), "FOO");
  EXPECT_STREQ(enumName((EnumStrict)3), "BAR");
  EXPECT_STREQ(enumName((EnumStrict)3, "NOTBAR"), "BAR");
  EXPECT_STREQ(enumName((EnumStrict)42, "Universe"), "Universe");
  EXPECT_EQ(enumName((EnumStrict)42), nullptr);
  EXPECT_EQ(enumName((EnumStrict)-1), nullptr);
}

TEST(EnumUtilsTest, ShortEnumNameSafe) {
  EXPECT_EQ(enumNameSafe((EnumStrict)0), "UNKNOWN");
  EXPECT_EQ(enumNameSafe((EnumStrict)1), "VALUE");
  EXPECT_EQ(enumNameSafe((EnumStrict)2), "FOO");
  EXPECT_EQ(enumNameSafe((EnumStrict)3), "BAR");
  EXPECT_EQ(enumNameSafe((EnumStrict)42), "42");
  EXPECT_EQ(enumNameSafe((EnumStrict)-1), "-1");
}
