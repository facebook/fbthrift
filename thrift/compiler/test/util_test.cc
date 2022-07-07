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

TEST_F(UtilTest, scope_guard_empty) {
  make_scope_guard([] {});
}

TEST_F(UtilTest, scope_guard_full) {
  size_t called = 0;
  {
    auto g = make_scope_guard([&] { ++called; });
    EXPECT_EQ(0, called);
  }
  EXPECT_EQ(1, called);
}

TEST_F(UtilTest, scope_guard_full_move_ctor) {
  size_t called = 0;
  {
    auto g = make_scope_guard([&] { ++called; });
    EXPECT_EQ(0, called);
    auto gg = std::move(g);
    EXPECT_EQ(0, called);
  }
  EXPECT_EQ(1, called);
}

TEST_F(UtilTest, scope_guard_full_dismiss) {
  size_t called = 0;
  {
    auto g = make_scope_guard([&] { ++called; });
    EXPECT_EQ(0, called);
    g.dismiss();
    EXPECT_EQ(0, called);
  }
  EXPECT_EQ(0, called);
}

TEST_F(UtilTest, scope_guard_full_supports_move_only) {
  size_t called = 0;
  {
    make_scope_guard([&, p = std::make_unique<int>(3)] { called += *p; });
  }
  EXPECT_EQ(3, called);
}

TEST_F(UtilTest, scope_guard_throws_death) {
  static constexpr auto msg = "some secret text that no one knows";
  auto func = [=] { throw std::runtime_error(msg); };
  EXPECT_NO_THROW(make_scope_guard(func).dismiss()) << "sanity check";
  EXPECT_DEATH(make_scope_guard(func), msg);
}
