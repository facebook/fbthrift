/*
 * Copyright 2016 Facebook, Inc.
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

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <thrift/compiler/generate/common.h>

TEST(GenerateCommon, SplitNamespace) {
  const std::vector<std::string> namespaces{
      "",
      "this",
      "this.is",
      "this.is.valid",
  };

  const std::vector<std::vector<std::string>> expected{
      {},
      {"this"},
      {"this", "is"},
      {"this", "is", "valid"},
  };

  std::vector<std::vector<std::string>> splits;
  for (const auto& ns : namespaces) {
    splits.push_back(split_namespace(ns));
  }

  EXPECT_EQ(expected, splits);
}

TEST(GenerateCommon, EscapeQuotes) {
  std::vector<std::string> quotedstrings{
      R"(no quotes)",
      R"("quotes")",
      R"({"a": 1, "b": -2, "c": -3})",
  };

  const std::vector<std::string> expected{
      R"(no quotes)",
      R"(\"quotes\")",
      R"({\"a\": 1, \"b\": -2, \"c\": -3})",
  };

  std::vector<std::string> escaped;
  for (auto& s : quotedstrings) {
    escape_quotes_cpp(s);
    escaped.push_back(s);
  }

  EXPECT_EQ(expected, escaped);
}

TEST(GenerateCommon, TrimWhitespace) {
  std::vector<std::string> whitespaces{
      "    ",
      "   left",
      "right ",
      "   both spaces ",
  };

  const std::vector<std::string> expected{
      "",
      "left",
      "right",
      "both spaces",
  };

  std::vector<std::string> trimmed;
  for (auto& s : whitespaces) {
    trim_whitespace(s);
    trimmed.push_back(s);
  }

  EXPECT_EQ(expected, trimmed);
}
