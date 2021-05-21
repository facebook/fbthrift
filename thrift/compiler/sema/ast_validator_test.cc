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

#include <memory>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_enum.h>
#include <thrift/compiler/ast/t_enum_value.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_function.h>
#include <thrift/compiler/ast/t_interaction.h>
#include <thrift/compiler/ast/t_paramlist.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_service.h>
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

TEST_F(StdAstValidatorTest, InterfaceNamesUniqueNoError) {
  // Create interfaces with non-overlapping functions.
  {
    auto service = std::make_unique<t_service>(&program_, "Service");
    service->add_function(std::make_unique<t_function>(
        &t_base_type::t_void(),
        "bar",
        std::make_unique<t_paramlist>(&program_)));
    service->add_function(std::make_unique<t_function>(
        &t_base_type::t_void(),
        "baz",
        std::make_unique<t_paramlist>(&program_)));
    program_.add_service(std::move(service));
  }

  {
    auto interaction =
        std::make_unique<t_interaction>(&program_, "Interaction");
    interaction->add_function(std::make_unique<t_function>(
        &t_base_type::t_void(),
        "bar",
        std::make_unique<t_paramlist>(&program_)));
    interaction->add_function(std::make_unique<t_function>(
        &t_base_type::t_void(),
        "baz",
        std::make_unique<t_paramlist>(&program_)));
    program_.add_interaction(std::move(interaction));
  }

  // No errors will be found
  EXPECT_THAT(validate(), ::testing::IsEmpty());
}

TEST_F(StdAstValidatorTest, ReapeatedNamesInService) {
  // Create interfaces with overlapping functions.
  {
    auto service = std::make_unique<t_service>(&program_, "Service");
    auto fn1 = std::make_unique<t_function>(
        &t_base_type::t_void(),
        "foo",
        std::make_unique<t_paramlist>(&program_));
    fn1->set_lineno(1);
    auto fn2 = std::make_unique<t_function>(
        &t_base_type::t_void(),
        "foo",
        std::make_unique<t_paramlist>(&program_));
    fn2->set_lineno(2);
    service->add_function(std::move(fn1));
    service->add_function(std::move(fn2));
    service->add_function(std::make_unique<t_function>(
        &t_base_type::t_void(),
        "bar",
        std::make_unique<t_paramlist>(&program_)));
    program_.add_service(std::move(service));
  }

  {
    auto interaction =
        std::make_unique<t_interaction>(&program_, "Interaction");
    auto fn1 = std::make_unique<t_function>(
        &t_base_type::t_void(),
        "bar",
        std::make_unique<t_paramlist>(&program_));
    fn1->set_lineno(3);
    auto fn2 = std::make_unique<t_function>(
        &t_base_type::t_void(),
        "bar",
        std::make_unique<t_paramlist>(&program_));
    fn2->set_lineno(4);

    interaction->add_function(std::make_unique<t_function>(
        &t_base_type::t_void(),
        "foo",
        std::make_unique<t_paramlist>(&program_)));
    interaction->add_function(std::move(fn1));
    interaction->add_function(std::move(fn2));
    program_.add_interaction(std::move(interaction));
  }

  EXPECT_THAT(
      validate(),
      ::testing::UnorderedElementsAre(
          diagnostic{
              diagnostic_level::failure,
              "Function `foo` is already defined in `Service`.",
              "/path/to/file.thrift",
              2},
          diagnostic{
              diagnostic_level::failure,
              "Function `bar` is already defined in `Interaction`.",
              "/path/to/file.thrift",
              4}));
}

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

TEST_F(StdAstValidatorTest, UnsetEnumValues) {
  auto tenum = std::make_unique<t_enum>(&program_, "foo");
  auto tenum_ptr = tenum.get();
  program_.add_enum(std::move(tenum));

  auto enum_value_1 = std::make_unique<t_enum_value>("Foo", 1);
  auto enum_value_2 = std::make_unique<t_enum_value>("Bar");
  auto enum_value_3 = std::make_unique<t_enum_value>("Baz");
  enum_value_1->set_lineno(1);
  enum_value_2->set_lineno(2);
  enum_value_3->set_lineno(3);
  tenum_ptr->append(std::move(enum_value_1));
  tenum_ptr->append(std::move(enum_value_2));
  tenum_ptr->append(std::move(enum_value_3));

  // Bar and Baz will have errors.
  EXPECT_THAT(
      validate(),
      ::testing::UnorderedElementsAre(
          diagnostic{
              diagnostic_level::failure,
              "The enum value, `Bar`, must have an explicitly assigned value.",
              "/path/to/file.thrift",
              2},
          diagnostic{
              diagnostic_level::failure,
              "The enum value, `Baz`, must have an explicitly assigned value.",
              "/path/to/file.thrift",
              3}));
}

TEST_F(StdAstValidatorTest, QualifiedInUnion) {
  auto tunion = std::make_unique<t_union>(&program_, "Union");
  tunion->set_lineno(1);
  {
    auto field = std::make_unique<t_field>(&t_base_type::t_i64(), "req", 1);
    field->set_lineno(2);
    field->set_qualifier(t_field_qualifier::required);
    tunion->append(std::move(field));
  }
  {
    auto field = std::make_unique<t_field>(&t_base_type::t_i64(), "op", 2);
    field->set_lineno(3);
    field->set_qualifier(t_field_qualifier::optional);
    tunion->append(std::move(field));
  }
  {
    auto field = std::make_unique<t_field>(&t_base_type::t_i64(), "non", 3);
    field->set_lineno(4);
    tunion->append(std::move(field));
  }
  program_.add_struct(std::move(tunion));

  // Qualified fields will have errors.
  EXPECT_THAT(
      validate(),
      ::testing::UnorderedElementsAre(
          diagnostic{
              diagnostic_level::failure,
              "Unions cannot contain qualified fields. Remove `required` qualifier from field `req`.",
              "/path/to/file.thrift",
              2},
          diagnostic{
              diagnostic_level::failure,
              "Unions cannot contain qualified fields. Remove `optional` qualifier from field `op`.",
              "/path/to/file.thrift",
              3}));
}

} // namespace
} // namespace apache::thrift::compiler
