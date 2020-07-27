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

#include <memory>
#include <string>
#include <vector>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/compiler/ast/t_function.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/test/parser_test_helpers.h>
#include <thrift/compiler/validator/validator.h>

namespace {

class ValidatorTest : public testing::Test {};

} // namespace

TEST_F(ValidatorTest, run_validator) {
  class fake_validator : public validator {
   public:
    using visitor::visit;
    bool visit(t_program* const program) final {
      EXPECT_TRUE(validator::visit(program));
      add_error(50, "sadface");
      return true;
    }
  };

  t_program program("/path/to/file.thrift");
  auto errors = run_validator<fake_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ("[FAILURE:/path/to/file.thrift:50] sadface", errors.front().str());
}

TEST_F(ValidatorTest, ServiceNamesUniqueNoError) {
  // Create a service with non-overlapping functions
  auto service = create_fake_service("foo");
  auto fn1 = create_fake_function<int(int)>("bar");
  auto fn2 = create_fake_function<void(double)>("baz");
  service->add_function(std::move(fn1));
  service->add_function(std::move(fn2));

  t_program program("/path/to/file.thrift");
  program.add_service(std::move(service));

  // No errors will be found
  auto errors =
      run_validator<service_method_name_uniqueness_validator>(&program);
  EXPECT_TRUE(errors.empty());
}

TEST_F(ValidatorTest, ReapeatedNamesInService) {
  // Create a service with two overlapping functions
  auto service = create_fake_service("bar");
  auto fn1 = create_fake_function<void(int, int)>("foo");
  auto fn2 = create_fake_function<int(double)>("foo");
  fn2->set_lineno(1);
  service->add_function(std::move(fn1));
  service->add_function(std::move(fn2));

  t_program program("/path/to/file.thrift");
  program.add_service(std::move(service));

  // An error will be found
  const std::string expected =
      "[FAILURE:/path/to/file.thrift:1] "
      "Function bar.foo redefines bar.foo";
  auto errors =
      run_validator<service_method_name_uniqueness_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ(expected, errors.front().str());
}

TEST_F(ValidatorTest, RepeatedNameInExtendedService) {
  // Create first service with two non repeated functions
  auto service_1 = create_fake_service("bar");
  auto fn1 = create_fake_function<void(int)>("baz");
  auto fn2 = create_fake_function<void(int, int)>("foo");
  service_1->add_function(std::move(fn1));
  service_1->add_function(std::move(fn2));

  // Create second service extending the first servie and no repeated function
  auto service_2 = create_fake_service("qux");
  auto service_2_ptr = service_2.get();
  service_2->set_extends(service_1.get());
  auto fn3 = create_fake_function<void()>("mos");
  service_2->add_function(std::move(fn3));

  t_program program("/path/to/file.thrift");
  program.add_service(std::move(service_1));
  program.add_service(std::move(service_2));

  // No errors will be found
  auto errors =
      run_validator<service_method_name_uniqueness_validator>(&program);
  EXPECT_TRUE(errors.empty());

  // Add an overlapping function in the second service
  auto fn4 = create_fake_function<void(double)>("foo");
  fn4->set_lineno(1);
  service_2_ptr->add_function(std::move(fn4));

  // An error will be found
  const std::string expected =
      "[FAILURE:/path/to/file.thrift:1] "
      "Function qux.foo redefines service bar.foo";
  errors = run_validator<service_method_name_uniqueness_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ(expected, errors.front().str());
}

TEST_F(ValidatorTest, RepeatedNamesInEnumValues) {
  auto tenum = create_fake_enum("foo");
  auto tenum_ptr = tenum.get();

  t_program program("/path/to/file.thrift");
  program.add_enum(std::move(tenum));

  tenum_ptr->append(std::make_unique<t_enum_value>("bar", 1), nullptr);
  tenum_ptr->append(std::make_unique<t_enum_value>("not_bar", 2), nullptr);

  // No errors will be found
  auto errors = run_validator<enum_value_names_uniqueness_validator>(&program);
  EXPECT_TRUE(errors.empty());

  // Add enum with repeated value
  auto enum_value_3 = std::make_unique<t_enum_value>("bar", 3);
  enum_value_3->set_lineno(1);
  tenum_ptr->append(std::move(enum_value_3), nullptr);

  // An error will be found
  const std::string expected =
      "[FAILURE:/path/to/file.thrift:1] "
      "Redefinition of value bar in enum foo";
  errors = run_validator<enum_value_names_uniqueness_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ(expected, errors.front().str());
}

TEST_F(ValidatorTest, DuplicatedEnumValues) {
  auto tenum = create_fake_enum("foo");
  auto tenum_ptr = tenum.get();

  t_program program("/path/to/file.thrift");
  program.add_enum(std::move(tenum));

  auto enum_value_1 = std::make_unique<t_enum_value>("bar", 1);
  auto enum_value_2 = std::make_unique<t_enum_value>("foo", 1);
  enum_value_2->set_lineno(1);
  tenum_ptr->append(std::move(enum_value_1), nullptr);
  tenum_ptr->append(std::move(enum_value_2), nullptr);

  // An error will be found
  const std::string expected =
      "[FAILURE:/path/to/file.thrift:1] "
      "Duplicate value foo=1 with value bar in enum foo.";
  auto errors = run_validator<enum_values_uniqueness_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ(expected, errors.front().str());
}

TEST_F(ValidatorTest, UnsetEnumValues) {
  auto tenum = create_fake_enum("Foo");
  auto tenum_ptr = tenum.get();

  t_program program("/path/to/file.thrift");
  program.add_enum(std::move(tenum));

  auto enum_value_1 = std::make_unique<t_enum_value>("Bar");
  auto enum_value_2 = std::make_unique<t_enum_value>("Baz");
  enum_value_1->set_lineno(2);
  enum_value_2->set_lineno(3);
  tenum_ptr->append(std::move(enum_value_1), nullptr);
  tenum_ptr->append(std::move(enum_value_2), nullptr);

  // An error will be found
  auto errors = run_validator<enum_values_set_validator>(&program);
  EXPECT_EQ(2, errors.size());
  EXPECT_EQ(
      "[FAILURE:/path/to/file.thrift:2] Unset enum value Bar in enum Foo. "
      "Add an explicit value to suppress this error",
      errors.at(0).str());
  EXPECT_EQ(
      "[FAILURE:/path/to/file.thrift:3] Unset enum value Baz in enum Foo. "
      "Add an explicit value to suppress this error",
      errors.at(1).str());
}

TEST_F(ValidatorTest, QualifiedInUnion) {
  t_program program("/path/to/file.thrift");
  t_base_type i64type("i64", t_base_type::TYPE_I64);

  auto field = std::make_unique<t_field>(&i64type, "foo", 1);
  field->set_lineno(5);
  field->set_req(t_field::T_REQUIRED);

  auto struct_union = std::make_unique<t_struct>(&program, "Bar");
  struct_union->set_union(true);

  struct_union->append(std::move(field));

  field = std::make_unique<t_field>(&i64type, "baz", 1);
  field->set_lineno(6);
  field->set_req(t_field::T_OPTIONAL);
  struct_union->append(std::move(field));

  field = std::make_unique<t_field>(&i64type, "qux", 1);
  field->set_lineno(7);
  struct_union->append(std::move(field));

  program.add_struct(std::move(struct_union));

  auto errors = run_validator<union_no_qualified_fields_validator>(&program);
  EXPECT_EQ(2, errors.size());
  EXPECT_EQ(
      "[FAILURE:/path/to/file.thrift:5] Unions cannot contain qualified fields. "
      "Remove required qualifier from field 'foo'",
      errors.front().str());
  EXPECT_EQ(
      "[FAILURE:/path/to/file.thrift:6] Unions cannot contain qualified fields. "
      "Remove optional qualifier from field 'baz'",
      errors.at(1).str());
}

TEST_F(ValidatorTest, DuplicatedStructNames) {
  t_program program("/path/to/file.thrift");

  program.add_struct(std::make_unique<t_struct>(&program, "Foo"));
  program.add_struct(std::make_unique<t_struct>(&program, "Bar"));
  auto ex = std::make_unique<t_struct>(&program, "Foo");
  ex->set_xception(true);
  ex->set_lineno(42);
  program.add_xception(std::move(ex));

  auto errors = run_validator<struct_names_uniqueness_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ(
      "[FAILURE:/path/to/file.thrift:42] Redefinition of type `Foo`",
      errors.front().str());
}
