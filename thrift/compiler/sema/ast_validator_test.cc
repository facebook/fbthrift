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

#include <thrift/compiler/sema/ast_validator.h>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/compiler/sema/diagnostic.h>
#include <thrift/compiler/sema/diagnostic_context.h>

namespace apache::thrift::compiler {
namespace {

class AstValidatorTest : public ::testing::Test {};

TEST_F(AstValidatorTest, Output) {
  ast_validator validator;
  validator.add_program_visitor(
      [](diagnostic_context& ctx, const t_program* program) {
        ctx.info(program, "test");
      });

  t_program program("path/to/program.thrift");
  const t_program* cprogram = &program;
  diagnostic_results results;
  diagnostic_context ctx{results, diagnostic_params::keep_all()};
  ctx.start_program(&program);
  validator(ctx, cprogram);
  EXPECT_THAT(
      results.diagnostics(),
      ::testing::ElementsAre(
          diagnostic(diagnostic_level::info, "test", &program, &program)));
}

class StdAstValidatorTest : public ::testing::Test {
 protected:
  std::vector<diagnostic> validate() {
    diagnostic_results results;
    diagnostic_context ctx{results, diagnostic_params::keep_all()};
    ctx.start_program(&program_);
    standard_validator()(ctx, &program_);
    return std::move(results).diagnostics();
  }

  t_program program_{"/path/to/file.thrift"};
};

TEST_F(StdAstValidatorTest, DuplicatedEnumValues) {
  auto tenum = std::make_unique<t_enum>(&program_, "foo");
  auto tenum_ptr = tenum.get();
  program_.add_enum(std::move(tenum));

  tenum_ptr->append(std::make_unique<t_enum_value>("bar", 1));
  tenum_ptr->append(std::make_unique<t_enum_value>("baz", 2));

  // No errors will be found.
  EXPECT_THAT(validate(), ::testing::IsEmpty());

  // Add enum_value with repeated value.
  auto enum_value_3 = std::make_unique<t_enum_value>("foo", 1);
  enum_value_3->set_lineno(1);
  tenum_ptr->append(std::move(enum_value_3));

  // An error will be found.
  EXPECT_THAT(
      validate(),
      ::testing::UnorderedElementsAre(diagnostic{
          diagnostic_level::failure,
          "Duplicate value `foo=1` with value `bar` in enum `foo`.",
          program_.path(),
          1}));
}

TEST_F(StdAstValidatorTest, RepeatedNamesInEnumValues) {
  auto tenum = std::make_unique<t_enum>(&program_, "foo");
  auto tenum_ptr = tenum.get();
  program_.add_enum(std::move(tenum));

  tenum_ptr->append(std::make_unique<t_enum_value>("bar", 1));
  tenum_ptr->append(std::make_unique<t_enum_value>("not_bar", 2));

  // No errors will be found.
  EXPECT_THAT(validate(), ::testing::IsEmpty());

  // Add enum_value with repeated name.
  auto enum_value_3 = std::make_unique<t_enum_value>("bar", 3);
  enum_value_3->set_lineno(1);
  tenum_ptr->append(std::move(enum_value_3));

  // An error will be found.
  EXPECT_THAT(
      validate(),
      ::testing::UnorderedElementsAre(diagnostic{
          diagnostic_level::failure,
          "Redefinition of value `bar` in enum `foo`.",
          program_.path(),
          1}));
}

} // namespace
} // namespace apache::thrift::compiler
