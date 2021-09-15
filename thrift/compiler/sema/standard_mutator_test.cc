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

#include <thrift/compiler/sema/standard_mutator.h>

#include <folly/portability/GTest.h>
#include <thrift/compiler/ast/diagnostic.h>
#include <thrift/compiler/ast/diagnostic_context.h>

namespace apache::thrift::compiler {
namespace {

class StandardMutatorTest : public ::testing::Test {
 protected:
  std::vector<diagnostic> mutate(
      t_program& program,
      diagnostic_params params = diagnostic_params::keep_all()) {
    diagnostic_results results;
    diagnostic_context ctx{results, std::move(params)};
    ctx.start_program(&program);
    standard_mutator().mutate(ctx, program);
    return std::move(results).diagnostics();
  }
};

TEST_F(StandardMutatorTest, Empty) {
  t_program program("path/to/file.thrift");
  mutate(program);
}

} // namespace
} // namespace apache::thrift::compiler
