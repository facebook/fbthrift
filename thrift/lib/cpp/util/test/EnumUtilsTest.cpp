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

#include <gtest/gtest.h>

#include "thrift/lib/cpp/util/EnumUtils.h"
#include "thrift/lib/cpp/util/test/gen-cpp/enum_non_strict_types.h"
#include "thrift/lib/cpp/util/test/gen-cpp/enum_strict_types.h"

using namespace apache::thrift::util;

TEST(EnumStrict, ShortEnumName) {
  EXPECT_EQ(strcmp(shortEnumName((EnumStrict)0), "UNKNOWN"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumStrict)1), "VALUE"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumStrict)2), "FOO"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumStrict)3), "BAR"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumStrict)3, "NOTBAR"), "BAR"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumStrict)42, "Universe"), "Universe"), 0);
  EXPECT_EQ(shortEnumName((EnumStrict)42), nullptr);
  EXPECT_EQ(shortEnumName((EnumStrict)-1), nullptr);
}

TEST(EnumNonStrict, ShortEnumName) {
  EXPECT_EQ(strcmp(shortEnumName((EnumNonStrict)0), "UNKNOWN"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumNonStrict)1), "VALUE"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumNonStrict)2), "FOO"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumNonStrict)3), "BAR"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumNonStrict)6), "NEW_VALUE"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumStrict)3, "NOTBAR"), "BAR"), 0);
  EXPECT_EQ(strcmp(shortEnumName((EnumStrict)42, "Universe"), "Universe"), 0);
  EXPECT_EQ(shortEnumName((EnumNonStrict)4), nullptr);
  EXPECT_EQ(shortEnumName((EnumNonStrict)5), nullptr);
  EXPECT_EQ(shortEnumName((EnumNonStrict)42), nullptr);
  EXPECT_EQ(shortEnumName((EnumNonStrict)-1), nullptr);
}

TEST(EnumStrict, ShortEnumNameSafe) {
  EXPECT_EQ(shortEnumNameSafe((EnumStrict)0), "UNKNOWN");
  EXPECT_EQ(shortEnumNameSafe((EnumStrict)1), "VALUE");
  EXPECT_EQ(shortEnumNameSafe((EnumStrict)2), "FOO");
  EXPECT_EQ(shortEnumNameSafe((EnumStrict)3), "BAR");
  EXPECT_EQ(shortEnumNameSafe((EnumStrict)42), "42");
  EXPECT_EQ(shortEnumNameSafe((EnumStrict)-1), "-1");
}

TEST(EnumNonStrict, ShortEnumNameSafe) {
  EXPECT_EQ(shortEnumNameSafe((EnumNonStrict)0), "UNKNOWN");
  EXPECT_EQ(shortEnumNameSafe((EnumNonStrict)1), "VALUE");
  EXPECT_EQ(shortEnumNameSafe((EnumNonStrict)2), "FOO");
  EXPECT_EQ(shortEnumNameSafe((EnumNonStrict)3), "BAR");
  EXPECT_EQ(shortEnumNameSafe((EnumNonStrict)6), "NEW_VALUE");
  EXPECT_EQ(shortEnumNameSafe((EnumNonStrict)4), "4");
  EXPECT_EQ(shortEnumNameSafe((EnumNonStrict)5), "5");
  EXPECT_EQ(shortEnumNameSafe((EnumNonStrict)42), "42");
  EXPECT_EQ(shortEnumNameSafe((EnumNonStrict)-1), "-1");
}
