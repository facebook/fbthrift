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

#include <thrift/compiler/diagnostic.h>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

using namespace apache::thrift::compiler;

TEST(DiagnosticTest, Str) {
  EXPECT_EQ(
      diagnostic(diagnostic_level::debug, "m", "f", 1, "t").str(),
      "[DEBUG:f:1] m");
  EXPECT_EQ(
      diagnostic(diagnostic_level::failure, "m", "f", 0, "t").str(),
      "[FAILURE:f] m");
  EXPECT_EQ(
      diagnostic(diagnostic_level::info, "m", "f", 1).str(), "[INFO:f:1] m");
  EXPECT_EQ(
      diagnostic(diagnostic_level::parse_error, "m", "f").str(), "[ERROR:f] m");
  EXPECT_EQ(
      diagnostic(diagnostic_level::warning, "m", "").str(), "[WARNING:] m");
}

class DiagnosticsEngineTest : public ::testing::Test {
 public:
  source_manager source_mgr;
  diagnostic_results results;
  diagnostics_engine diags;
  source src;

  DiagnosticsEngineTest()
      : diags(source_mgr, results),
        src(source_mgr.add_string("path/to/file.thrift", "")) {}
};

TEST_F(DiagnosticsEngineTest, KeepDebug) {
  // Not reported by default.
  diags.report(src.start, diagnostic_level::debug, "hi");
  EXPECT_THAT(results.diagnostics(), testing::IsEmpty());

  diags.params().debug = true;
  diags.report(src.start, diagnostic_level::debug, "hi");
  EXPECT_THAT(
      results.diagnostics(),
      testing::ElementsAre(
          diagnostic(diagnostic_level::debug, "hi", "path/to/file.thrift", 1)));
}

TEST_F(DiagnosticsEngineTest, KeepInfo) {
  // Not reported by default.
  diags.report(src.start, diagnostic_level::info, "hi");
  EXPECT_THAT(results.diagnostics(), ::testing::IsEmpty());

  diags.params().info = true;
  diags.report(src.start, diagnostic_level::info, "hi");
  EXPECT_THAT(
      results.diagnostics(),
      ::testing::ElementsAre(
          diagnostic(diagnostic_level::info, "hi", "path/to/file.thrift", 1)));
}

TEST(DiagnosticResultsTest, Empty) {
  diagnostic_results results;
  EXPECT_FALSE(results.has_failure());
  for (int i = 0; i <= static_cast<int>(diagnostic_level::debug); ++i) {
    EXPECT_EQ(results.count(static_cast<diagnostic_level>(i)), 0);
  }
}

TEST(DiagnosticResultsTest, Count) {
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
