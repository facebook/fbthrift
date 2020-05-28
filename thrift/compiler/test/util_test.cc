/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

TEST(String, strip_left_margin_really_empty) {
  auto input = "";
  auto expected = "";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_empty) {
  auto input = R"TEXT(
  )TEXT";
  auto expected = "";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_only_whitespace) {
  //  using ~ as a marker
  std::string input = R"TEXT(
    ~
  )TEXT";
  input = boost::regex_replace(input, boost::regex("~"), "");
  EXPECT_EQ("\n    \n  ", input);
  auto expected = "\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_only_uneven_whitespace) {
  //  using ~ as a marker1
  std::string input = R"TEXT(
    ~
      ~
  )TEXT";
  input = boost::regex_replace(input, boost::regex("~"), "");
  EXPECT_EQ("\n    \n      \n  ", input);
  auto expected = "\n\n";

  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_one_line) {
  auto input = R"TEXT(
    hi there bob!
  )TEXT";
  auto expected = "hi there bob!\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_two_lines) {
  auto input = R"TEXT(
    hi there bob!
    nice weather today!
  )TEXT";
  auto expected = "hi there bob!\nnice weather today!\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_three_lines_uneven) {
  auto input = R"TEXT(
      hi there bob!
    nice weather today!
      so long!
  )TEXT";
  auto expected = "  hi there bob!\nnice weather today!\n  so long!\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_preceding_blank_lines) {
  auto input = R"TEXT(


    hi there bob!
  )TEXT";
  auto expected = "\n\nhi there bob!\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_succeeding_blank_lines) {
  auto input = R"TEXT(
    hi there bob!


  )TEXT";
  auto expected = "hi there bob!\n\n\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_interstitial_undented_whiteline) {
  //  using ~ as a marker
  std::string input = R"TEXT(
      hi there bob!
    ~
      so long!
  )TEXT";
  input = boost::regex_replace(input, boost::regex(" +~"), "");
  EXPECT_EQ("\n      hi there bob!\n\n      so long!\n  ", input);
  auto expected = "hi there bob!\n\nso long!\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_interstitial_dedented_whiteline) {
  //  using ~ as a marker
  std::string input = R"TEXT(
      hi there bob!
    ~
      so long!
  )TEXT";
  input = boost::regex_replace(input, boost::regex("~"), "");
  EXPECT_EQ("\n      hi there bob!\n    \n      so long!\n  ", input);
  auto expected = "hi there bob!\n\nso long!\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_interstitial_equidented_whiteline) {
  //  using ~ as a marker
  std::string input = R"TEXT(
      hi there bob!
      ~
      so long!
  )TEXT";
  input = boost::regex_replace(input, boost::regex("~"), "");
  EXPECT_EQ("\n      hi there bob!\n      \n      so long!\n  ", input);
  auto expected = "hi there bob!\n\nso long!\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_interstitial_indented_whiteline) {
  //  using ~ as a marker
  std::string input = R"TEXT(
      hi there bob!
        ~
      so long!
  )TEXT";
  input = boost::regex_replace(input, boost::regex("~"), "");
  EXPECT_EQ("\n      hi there bob!\n        \n      so long!\n  ", input);
  auto expected = "hi there bob!\n  \nso long!\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_no_pre_whitespace) {
  //  using ~ as a marker
  std::string input = R"TEXT(      hi there bob!
        ~
      so long!
  )TEXT";
  input = boost::regex_replace(input, boost::regex("~"), "");
  EXPECT_EQ("      hi there bob!\n        \n      so long!\n  ", input);
  auto expected = "hi there bob!\n  \nso long!\n";
  EXPECT_EQ(expected, strip_left_margin(input));
}

TEST(String, strip_left_margin_no_post_whitespace) {
  //  using ~ as a marker
  std::string input = R"TEXT(
      hi there bob!
        ~
      so long!  )TEXT";
  input = boost::regex_replace(input, boost::regex("~"), "");
  EXPECT_EQ("\n      hi there bob!\n        \n      so long!  ", input);
  auto expected = "hi there bob!\n  \nso long!  ";
  EXPECT_EQ(expected, strip_left_margin(input));
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

TEST_F(UtilTest, topological_sort_example) {
  std::map<std::string, std::set<std::string>> graph{
      {"e", {"c", "a"}},
      {"d", {"b", "c"}},
      {"c", {"d", "b", "a"}},
      {"b", {}},
      {"a", {"b"}},
  };
  std::vector<std::string> vertices;
  vertices.reserve(graph.size());
  for (const auto& kvp : graph) {
    vertices.push_back(kvp.first);
  }
  auto result = topological_sort<std::string>(
      vertices.begin(), vertices.end(), [&](auto item) {
        return std::vector<std::string>(graph[item].begin(), graph[item].end());
      });
  EXPECT_EQ(std::vector<std::string>({"b", "a", "d", "c", "e"}), result);
}
