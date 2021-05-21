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

#include <thrift/compiler/sema/diagnostic_context.h>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/compiler/sema/diagnostic.h>

namespace apache::thrift::compiler {
namespace {

class DiagnosticReporterTest : public ::testing::Test {
 public:
  DiagnosticReporterTest() : ctx_{results_}, program_{"path/to/file.thrift"} {}

  void SetUp() override { ctx_.start_program(&program_); }
  void TearDown() override { ctx_.end_program(&program_); }

 protected:
  diagnostic_results results_;
  diagnostic_context ctx_;
  t_program program_;
};

TEST_F(DiagnosticReporterTest, KeepDebug) {
  ctx_.debug(0, "", "hi");
  // Not reported by default.
  EXPECT_THAT(results_.diagnostics(), ::testing::IsEmpty());
  ctx_.params().debug = true;
  ctx_.debug(0, "", "hi");
  EXPECT_THAT(
      results_.diagnostics(),
      ::testing::ElementsAre(
          diagnostic{diagnostic_level::debug, "hi", "path/to/file.thrift"}));
}

TEST_F(DiagnosticReporterTest, KeepInfo) {
  ctx_.info(0, "", "hi");
  // Not reported by default.
  EXPECT_THAT(results_.diagnostics(), ::testing::IsEmpty());
  ctx_.params().info = true;
  ctx_.info(0, "", "hi");
  EXPECT_THAT(
      results_.diagnostics(),
      ::testing::ElementsAre(
          diagnostic{diagnostic_level::info, "hi", "path/to/file.thrift"}));
}

TEST_F(DiagnosticReporterTest, WarningLevel) {
  // Strict not reported by default.
  ctx_.warning(0, "", "hi");
  ctx_.warning_strict(0, "", "bye");
  EXPECT_THAT(
      results_.diagnostics(),
      ::testing::ElementsAre(
          diagnostic{diagnostic_level::warning, "hi", "path/to/file.thrift"}));
  results_ = {};

  // Not reported.
  ctx_.params().warn_level = 0;
  ctx_.warning(0, "", "hi");
  ctx_.warning_strict(0, "", "bye");
  EXPECT_THAT(results_.diagnostics(), ::testing::IsEmpty());

  // Both reported.
  ctx_.params().warn_level = 2;
  ctx_.warning(0, "", "hi");
  ctx_.warning_strict(0, "", "bye");
  EXPECT_THAT(
      results_.diagnostics(),
      ::testing::ElementsAre(
          diagnostic{diagnostic_level::warning, "hi", "path/to/file.thrift"},
          diagnostic{diagnostic_level::warning, "bye", "path/to/file.thrift"}));
}

} // namespace
} // namespace apache::thrift::compiler
