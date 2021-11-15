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

#include <thrift/compiler/ast/diagnostic_context.h>

#include <memory>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/compiler/ast/diagnostic.h>
#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache::thrift::compiler {
namespace {

class DiagnosticContextTest : public ::testing::Test {
 public:
  DiagnosticContextTest() : ctx_{results_}, program_{"path/to/file.thrift"} {}

  void SetUp() override { ctx_.start_program(&program_); }
  void TearDown() override { ctx_.end_program(&program_); }

 protected:
  diagnostic_results results_;
  diagnostic_context ctx_;
  t_program program_;
};

TEST_F(DiagnosticContextTest, KeepDebug) {
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

TEST_F(DiagnosticContextTest, KeepInfo) {
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

TEST_F(DiagnosticContextTest, WarningLevel) {
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

TEST_F(DiagnosticContextTest, NodeInfo) {
  ctx_.info(program_, "hi");
  // Not reported by default.
  EXPECT_THAT(results_.diagnostics(), ::testing::IsEmpty());
  ctx_.params().info = true;
  ctx_.info(program_, "hi");
  EXPECT_THAT(
      results_.diagnostics(),
      ::testing::ElementsAre(
          diagnostic{diagnostic_level::info, "hi", "path/to/file.thrift", -1}));
}

TEST_F(DiagnosticContextTest, NodeInfoWithName) {
  ctx_.info("DiagName", program_, "hi");
  // Not reported by default.
  EXPECT_THAT(results_.diagnostics(), ::testing::IsEmpty());
  ctx_.params().info = true;
  ctx_.info("DiagName", program_, "hi");
  EXPECT_THAT(
      results_.diagnostics(),
      ::testing::ElementsAre(diagnostic{
          diagnostic_level::info,
          "hi",
          "path/to/file.thrift",
          -1,
          "",
          "DiagName"}));
}

class NodeMetadataCacheTest : public ::testing::Test {};

TEST_F(NodeMetadataCacheTest, Cache) {
  node_metadata_cache cache;
  EXPECT_EQ(cache.get<int>(t_base_type::t_bool()), 0);
  EXPECT_EQ(
      &cache.get<int>(t_base_type::t_bool()),
      &cache.get<int>(t_base_type::t_bool()));
  EXPECT_EQ(
      cache.get(
          t_base_type::t_i32(), []() { return std::make_unique<int>(1); }),
      1);
  EXPECT_NE(
      &cache.get<int>(t_base_type::t_bool()),
      &cache.get<int>(t_base_type::t_i32()));
  cache.get<int>(t_base_type::t_bool()) = 2;
  EXPECT_EQ(cache.get<int>(t_base_type::t_bool()), 2);
  EXPECT_EQ(cache.get<int>(t_base_type::t_i32()), 1);
  EXPECT_EQ(cache.get<float>(t_base_type::t_i32()), 0.0);
}

} // namespace
} // namespace apache::thrift::compiler
