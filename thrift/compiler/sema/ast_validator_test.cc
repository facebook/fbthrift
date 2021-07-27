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
#include <thrift/compiler/ast/diagnostic.h>
#include <thrift/compiler/ast/diagnostic_context.h>
#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_enum.h>
#include <thrift/compiler/ast/t_enum_value.h>
#include <thrift/compiler/ast/t_exception.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_function.h>
#include <thrift/compiler/ast/t_interaction.h>
#include <thrift/compiler/ast/t_paramlist.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_typedef.h>

namespace apache::thrift::compiler {
namespace {

using ::testing::UnorderedElementsAre;

class AstValidatorTest : public ::testing::Test {};

TEST_F(AstValidatorTest, Output) {
  ast_validator validator;
  validator.add_program_visitor(
      [](diagnostic_context& ctx, const t_program& program) {
        ctx.info(program, "test");
      });

  t_program program("path/to/program.thrift");
  diagnostic_results results;
  diagnostic_context ctx{results, diagnostic_params::keep_all()};
  ctx.start_program(&program);
  validator(ctx, program);
  EXPECT_THAT(
      results.diagnostics(),
      UnorderedElementsAre(
          diagnostic(diagnostic_level::info, "test", &program, &program)));
}

class StdAstValidatorTest : public ::testing::Test {
 protected:
  std::vector<diagnostic> validate(
      diagnostic_params params = diagnostic_params::keep_all()) {
    diagnostic_results results;
    diagnostic_context ctx{results, std::move(params)};
    ctx.start_program(&program_);
    standard_validator()(ctx, program_);
    return std::move(results).diagnostics();
  }

  std::unique_ptr<t_const> inst(const t_struct* ttype, int lineno) {
    auto value = std::make_unique<t_const_value>();
    value->set_map();
    value->set_ttype(t_type_ref::from_ptr(ttype));
    auto result =
        std::make_unique<t_const>(&program_, ttype, "", std::move(value));
    result->set_lineno(lineno);
    return result;
  }

  diagnostic failure(int lineno, const std::string& msg) {
    return {diagnostic_level::failure, msg, "/path/to/file.thrift", lineno};
  }

  diagnostic warning(int lineno, const std::string& msg) {
    return {diagnostic_level::warning, msg, "/path/to/file.thrift", lineno};
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
          failure(2, "Function `foo` is already defined for `Service`."),
          failure(4, "Function `bar` is already defined for `Interaction`.")));
}

TEST_F(StdAstValidatorTest, RepeatedNameInExtendedService) {
  // Create first service with non repeated functions
  auto base = std::make_unique<t_service>(&program_, "Base");
  base->add_function(std::make_unique<t_function>(
      &t_base_type::t_void(), "bar", std::make_unique<t_paramlist>(&program_)));
  base->add_function(std::make_unique<t_function>(
      &t_base_type::t_void(), "baz", std::make_unique<t_paramlist>(&program_)));
  auto derived = std::make_unique<t_service>(&program_, "Derived", base.get());
  derived->add_function(std::make_unique<t_function>(
      &t_base_type::t_void(), "foo", std::make_unique<t_paramlist>(&program_)));

  auto derived_ptr = derived.get();
  program_.add_service(std::move(base));
  program_.add_service(std::move(derived));

  EXPECT_THAT(validate(), ::testing::IsEmpty());

  // Add an overlapping function in the derived service.
  auto dupe = std::make_unique<t_function>(
      &t_base_type::t_void(), "baz", std::make_unique<t_paramlist>(&program_));
  dupe->set_lineno(1);
  derived_ptr->add_function(std::move(dupe));

  // An error will be found
  EXPECT_THAT(
      validate(),
      ::testing::UnorderedElementsAre(failure(
          1, "Function `Derived.baz` redefines `service file.Base.baz`.")));
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
      UnorderedElementsAre(failure(
          1, "Duplicate value `foo=1` with value `bar` in enum `foo`.")));
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
      UnorderedElementsAre(
          failure(1, "Enum value `bar` is already defined for `foo`.")));
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
      UnorderedElementsAre(
          failure(
              2,
              "The enum value, `Bar`, must have an explicitly assigned value."),
          failure(
              3,
              "The enum value, `Baz`, must have an explicitly assigned value.")));
}

TEST_F(StdAstValidatorTest, UnionFieldAttributes) {
  auto tstruct = std::make_unique<t_struct>(&program_, "Struct");
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
  {
    auto field = std::make_unique<t_field>(tstruct.get(), "mixin", 4);
    field->set_lineno(5);
    field->set_annotation("cpp.mixin");
    tunion->append(std::move(field));
  }
  program_.add_struct(std::move(tstruct));
  program_.add_struct(std::move(tunion));

  EXPECT_THAT(
      validate(),
      UnorderedElementsAre(
          // Qualified fields will have errors.
          failure(
              2,
              "Unions cannot contain qualified fields. Remove `required` qualifier from field `req`."),
          failure(
              3,
              "Unions cannot contain qualified fields. Remove `optional` qualifier from field `op`."),
          // Fields with cpp.mixing have errors.
          failure(5, "Union `Union` cannot contain mixin field `mixin`.")));
}

TEST_F(StdAstValidatorTest, FieldId) {
  auto tstruct = std::make_unique<t_struct>(&program_, "Struct");
  tstruct->append(
      std::make_unique<t_field>(t_base_type::t_i64(), "explicit_id", 1));
  tstruct->append(
      std::make_unique<t_field>(t_base_type::t_i64(), "zero_id", 0));
  tstruct->append(
      std::make_unique<t_field>(t_base_type::t_i64(), "neg_id", -1));
  tstruct->append(std::make_unique<t_field>(
      t_base_type::t_i64(), "implicit_id", -2, false));

  program_.add_struct(std::move(tstruct));
  EXPECT_THAT(
      validate(),
      UnorderedElementsAre(
          failure(-1, "Zero value (0) not allowed as a field id for `zero_id`"),
          warning(
              -1,
              "No field id specified for `implicit_id`, resulting protocol may have conflicts or not be backwards compatible!")));
}

TEST_F(StdAstValidatorTest, MixinFieldType) {
  auto tstruct = std::make_unique<t_struct>(&program_, "Struct");
  auto tunion = std::make_unique<t_union>(&program_, "Union");
  auto texception = std::make_unique<t_exception>(&program_, "Exception");

  auto foo = std::make_unique<t_struct>(&program_, "Foo");
  {
    auto field = std::make_unique<t_field>(tstruct.get(), "struct_field", 1);
    field->set_lineno(1);
    field->set_annotation("cpp.mixin");
    field->set_qualifier(t_field_qualifier::optional);
    foo->append(std::move(field));
  }
  {
    auto field = std::make_unique<t_field>(tunion.get(), "union_field", 2);
    field->set_lineno(2);
    field->set_annotation("cpp.mixin");
    field->set_qualifier(t_field_qualifier::required);
    foo->append(std::move(field));
  }
  {
    auto field = std::make_unique<t_field>(texception.get(), "except_field", 3);
    field->set_lineno(3);
    field->set_annotation("cpp.mixin");
    foo->append(std::move(field));
  }
  {
    auto field =
        std::make_unique<t_field>(&t_base_type::t_i32(), "other_field", 4);
    field->set_lineno(4);
    field->set_annotation("cpp.mixin");
    foo->append(std::move(field));
  }

  program_.add_struct(std::move(tstruct));
  program_.add_struct(std::move(tunion));
  program_.add_exception(std::move(texception));
  program_.add_struct(std::move(foo));

  EXPECT_THAT(
      validate(diagnostic_params::only_failures()),
      UnorderedElementsAre(
          failure(1, "Mixin field `struct_field` cannot be optional."),
          failure(
              3,
              "Mixin field `except_field` type must be a struct or union. Found `Exception`."),
          failure(
              4,
              "Mixin field `other_field` type must be a struct or union. Found `i32`.")));
}

TEST_F(StdAstValidatorTest, RepeatedStructuredAnnotation) {
  auto foo = std::make_unique<t_struct>(&program_, "Foo");
  // A different program with the same name.
  t_program other_program("/path/to/other/file.thrift");
  // A different foo with the same file.name
  auto other_foo = std::make_unique<t_struct>(&other_program, "Foo");
  EXPECT_EQ(foo->get_full_name(), other_foo->get_full_name());

  t_scope scope;
  auto bar = std::make_unique<t_typedef>(
      &program_, &t_base_type::t_i32(), "Bar", &scope);
  bar->add_structured_annotation(inst(other_foo.get(), 1));
  bar->add_structured_annotation(inst(foo.get(), 2));
  bar->add_structured_annotation(inst(foo.get(), 3));

  program_.add_struct(std::move(foo));
  program_.add_typedef(std::move(bar));
  other_program.add_struct(std::move(other_foo));

  // Only the third annotation is a duplicate.
  EXPECT_THAT(
      validate(diagnostic_params::only_failures()),
      UnorderedElementsAre(failure(
          3, "Structured annotation `Foo` is already defined for `Bar`.")));
}

} // namespace
} // namespace apache::thrift::compiler
