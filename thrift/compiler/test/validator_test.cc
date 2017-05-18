/*
 * Copyright 2016-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/compiler/validator.h>

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thrift/compiler/parse/t_function.h>
#include <thrift/compiler/parse/t_service.h>
#include <thrift/compiler/test/parser_test_helpers.h>

using namespace apache::thrift::compiler;

namespace {

class ValidatorTest : public testing::Test{};

}

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
  EXPECT_THAT(errors, testing::ElementsAre(
        "[FAILURE:/path/to/file.thrift:50] sadface"));
}

TEST_F(ValidatorTest, ServiceNamesUniqueNoError) {
  // Create a service with non-overlapping functions
  auto service = create_fake_service("foo");
  auto fn1 = create_fake_function<int(int)>("bar");
  auto fn2 = create_fake_function<void(double)>("baz");
  service->add_function(fn1.get());
  service->add_function(fn2.get());

  t_program program("/path/to/file.thrift");
  program.add_service(service.get());

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
  service->add_function(fn1.get());
  service->add_function(fn2.get());

  t_program program("/path/to/file.thrift");
  program.add_service(service.get());

  // An error will be found
  const std::string expected = "[FAILURE:/path/to/file.thrift:1] "
    "Function bar.foo redefines bar.foo";
  auto errors =
    run_validator<service_method_name_uniqueness_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ(expected, errors.front());
}

TEST_F(ValidatorTest, RepeatedNameInExtendedService) {
  // Create first service with two non repeated functions
  auto service_1 = create_fake_service("bar");
  auto fn1 = create_fake_function<void(int)>("baz");
  auto fn2 = create_fake_function<void(int, int)>("foo");
  service_1->add_function(fn1.get());
  service_1->add_function(fn2.get());

  // Create second service extending the first servie and no repeated function
  auto service_2 = create_fake_service("qux");
  service_2->set_extends(service_1.get());
  auto fn3 = create_fake_function<void()>("mos");
  service_2->add_function(fn3.get());

  t_program program("/path/to/file.thrift");
  program.add_service(service_1.get());
  program.add_service(service_2.get());

  // No errors will be found
  auto errors =
    run_validator<service_method_name_uniqueness_validator>(&program);
  EXPECT_TRUE(errors.empty());

  // Add an overlapping function in the second service
  auto fn4 = create_fake_function<void(double)>("foo");
  fn4->set_lineno(1);
  service_2->add_function(fn4.get());

  // An error will be found
  const std::string expected = "[FAILURE:/path/to/file.thrift:1] "
    "Function qux.foo redefines service bar.foo";
  errors = run_validator<service_method_name_uniqueness_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ(expected, errors.front());
}

TEST_F(ValidatorTest, RepeatedNamesInEnumValues) {
  auto tenum = create_fake_enum("foo");

  t_program program("/path/to/file.thrift");
  program.add_enum(tenum.get());

  t_enum_value enum_value_1("bar", 1);
  t_enum_value enum_value_2("not_bar", 2);
  tenum->append(&enum_value_1);
  tenum->append(&enum_value_2);

  // No errors will be found
  auto errors =
    run_validator<enum_value_names_uniqueness_validator>(&program);
  EXPECT_TRUE(errors.empty());

  // Add enum with repeated value
  t_enum_value enum_value_3("bar", 3);
  enum_value_3.set_lineno(1);
  tenum->append(&enum_value_3);

  // An error will be found
  const std::string expected = "[FAILURE:/path/to/file.thrift:1] "
    "Redefinition of value bar in enum foo";
  errors =
    run_validator<enum_value_names_uniqueness_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ(expected, errors.front());
}

TEST_F(ValidatorTest, DuplicatedEnumValues) {
  auto tenum = create_fake_enum("foo");

  t_program program("/path/to/file.thrift");
  program.add_enum(tenum.get());

  t_enum_value enum_value_1("bar", 1);
  t_enum_value enum_value_2("foo", 1);
  enum_value_2.set_lineno(1);
  tenum->append(&enum_value_1);
  tenum->append(&enum_value_2);

  // An error will be found
  const std::string expected =
      "[FAILURE:/path/to/file.thrift:1] "
      "Duplicate value foo=1 with value bar in enum foo. "
      "Add thrift.duplicate_values annotation to enum to suppress this error";
  auto errors = run_validator<enum_values_uniqueness_validator>(&program);
  EXPECT_EQ(1, errors.size());
  EXPECT_EQ(expected, errors.front());

  // Now check that opt-out mechanism works
  tenum->annotations_["thrift.duplicate_values"] = "";
  errors = run_validator<enum_values_uniqueness_validator>(&program);
  EXPECT_TRUE(errors.empty());
}

TEST_F(ValidatorTest, UnsetEnumValues) {
  auto tenum = create_fake_enum("Foo");

  t_program program("/path/to/file.thrift");
  program.add_enum(tenum.get());

  t_enum_value enum_value_1("Bar");
  t_enum_value enum_value_2("Baz");
  enum_value_1.set_lineno(2);
  enum_value_2.set_lineno(3);
  tenum->append(&enum_value_1);
  tenum->append(&enum_value_2);

  // An error will be found
  auto errors = run_validator<enum_values_set_validator>(&program);
  EXPECT_THAT(
      errors,
      testing::ElementsAre(
          "[FAILURE:/path/to/file.thrift:2] Unset enum value Bar in enum Foo. "
          "Add an explicit value to suppress this error",
          "[FAILURE:/path/to/file.thrift:3] Unset enum value Baz in enum Foo. "
          "Add an explicit value to suppress this error"));
}
