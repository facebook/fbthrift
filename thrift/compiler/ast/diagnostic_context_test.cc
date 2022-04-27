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

#include <thrift/compiler/ast/diagnostic_context.h>

#include <memory>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_type.h>
#include <thrift/compiler/diagnostic.h>

namespace apache::thrift::compiler {
namespace {

class DiagnosticContextTest : public ::testing::Test {
 public:
  DiagnosticContextTest()
      : ctx_(source_mgr_, results_), program_("path/to/file.thrift") {}

  void SetUp() override { ctx_.begin_visit(program_); }
  void TearDown() override { ctx_.end_visit(program_); }

 protected:
  source_manager source_mgr_;
  diagnostic_results results_;
  diagnostic_context ctx_;
  t_program program_;
};

TEST_F(DiagnosticContextTest, WarningLevel) {
  // Strict not reported by default.
  ctx_.warning(0, "", "hi");
  ctx_.warning_legacy_strict(0, "", "bye");
  EXPECT_THAT(
      results_.diagnostics(),
      ::testing::ElementsAre(
          diagnostic{diagnostic_level::warning, "hi", "path/to/file.thrift"}));
  results_ = {};

  // Not reported.
  ctx_.params().warn_level = 0;
  ctx_.warning(0, "", "hi");
  ctx_.warning_legacy_strict(0, "", "bye");
  EXPECT_THAT(results_.diagnostics(), ::testing::IsEmpty());

  // Both reported.
  ctx_.params().warn_level = 2;
  ctx_.warning(0, "", "hi");
  ctx_.warning_legacy_strict(0, "", "bye");
  EXPECT_THAT(
      results_.diagnostics(),
      ::testing::ElementsAre(
          diagnostic{diagnostic_level::warning, "hi", "path/to/file.thrift"},
          diagnostic{diagnostic_level::warning, "bye", "path/to/file.thrift"}));
}

TEST(NodeMetadataCacheTest, Cache) {
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
