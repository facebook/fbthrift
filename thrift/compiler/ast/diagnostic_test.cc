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

#include <thrift/compiler/ast/diagnostic.h>

#include <folly/portability/GTest.h>

namespace apache::thrift::compiler {
namespace {

class DiagnosticTest : public ::testing::Test {};

TEST_F(DiagnosticTest, Str) {
  EXPECT_EQ(
      diagnostic(diagnostic_level::debug, "m", "f", 1, "t").str(),
      "[DEBUG:f:1] (last token was 't') m");
  EXPECT_EQ(
      diagnostic(diagnostic_level::failure, "m", "f", 0, "t").str(),
      "[FAILURE:f] (last token was 't') m");
  EXPECT_EQ(
      diagnostic(diagnostic_level::info, "m", "f", 1).str(), "[INFO:f:1] m");
  EXPECT_EQ(
      diagnostic(diagnostic_level::parse_error, "m", "f").str(), "[ERROR:f] m");
  EXPECT_EQ(
      diagnostic(diagnostic_level::warning, "m", "").str(), "[WARNING:] m");
}

class DiagnosticResultsTest : public ::testing::Test {};

TEST_F(DiagnosticResultsTest, Empty) {
  diagnostic_results results;
  EXPECT_FALSE(results.has_failure());
  for (int i = 0; i <= static_cast<int>(diagnostic_level::debug); ++i) {
    EXPECT_EQ(results.count(static_cast<diagnostic_level>(i)), 0);
  }
}

TEST_F(DiagnosticResultsTest, Count) {
  diagnostic_results results;
  for (int i = 0; i <= static_cast<int>(diagnostic_level::debug); ++i) {
    auto level = static_cast<diagnostic_level>(i);
    for (int j = 0; j <= i; ++j) {
      results.add({level, "hi", "file"});
    }
  }
  for (int i = 0; i <= static_cast<int>(diagnostic_level::debug); ++i) {
    EXPECT_EQ(results.count(static_cast<diagnostic_level>(i)), i + 1);
  }
}

} // namespace
} // namespace apache::thrift::compiler
