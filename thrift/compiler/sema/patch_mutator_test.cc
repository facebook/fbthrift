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

#include <memory>
#include <thrift/compiler/sema/patch_mutator.h>

#include <folly/portability/GTest.h>
#include <thrift/compiler/ast/diagnostic.h>
#include <thrift/compiler/ast/diagnostic_context.h>

namespace apache::thrift::compiler {
namespace {

class PatchGeneratorTest : public ::testing::Test {
 public:
  PatchGeneratorTest() : ctx_(results_) {}

  void SetUp() override {
    ctx_.start_program(&program_);
    ctx_.begin_visit(program_);
    gen_ = std::make_unique<patch_generator>(ctx_, program_);
  }

 protected:
  t_program program_{"path/to/file.thrift"};
  diagnostic_results results_;
  diagnostic_context ctx_;
  std::unique_ptr<patch_generator> gen_;
};

TEST_F(PatchGeneratorTest, Empty) {
  // We do not error when the patch types cannot be found.
  EXPECT_FALSE(results_.has_failure());
  // We should have gotten a warning per type.
  EXPECT_EQ(results_.count(diagnostic_level::warning), 9);
}

} // namespace
} // namespace apache::thrift::compiler
