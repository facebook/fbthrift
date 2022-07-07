/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/compiler/util.h>

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/regex.hpp>
#include <folly/portability/GTest.h>

using namespace apache::thrift::compiler;

class UtilTest : public testing::Test {};

TEST_F(UtilTest, exchange) {
  auto obj = std::map<std::string, int>{{"hello", 3}};
  auto old = exchange(obj, {{"world", 4}});
  EXPECT_EQ((std::map<std::string, int>{{"world", 4}}), obj);
  EXPECT_EQ((std::map<std::string, int>{{"hello", 3}}), old);
}

TEST_F(UtilTest, json_quote_ascii_string) {
  auto const input = "the\bquick\"brown\nfox\001jumps\201over";
  auto const expected = "\"the\\bquick\\\"brown\\nfox\\u0001jumps\\u0081over\"";
  auto actual = json_quote_ascii(input);
  EXPECT_EQ(expected, actual);
}

TEST_F(UtilTest, json_quote_ascii_stream) {
  auto const input = "the\bquick\"brown\nfox\001jumps\201over";
  auto const expected = "\"the\\bquick\\\"brown\\nfox\\u0001jumps\\u0081over\"";
  std::ostringstream actual;
  json_quote_ascii(actual, input);
  EXPECT_EQ(expected, actual.str());
}
