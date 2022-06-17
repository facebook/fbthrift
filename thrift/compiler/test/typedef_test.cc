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

#include <folly/portability/GTest.h>

#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_type.h>
#include <thrift/compiler/test/parser_test_helpers.h>

using namespace apache::thrift::compiler;

// Confirm that we don't crash when getting the true type of an unresolved
// type name (MissingType in the example below)
TEST(TypedefTest, bad_true_type) {
  auto source_mgr = source_manager();
  auto program = dedent_and_parse_to_program(source_mgr, R"(
    struct MyStruct {
      1: string first;
      2: MissingType second;
    }
  )");

  const std::vector<t_struct*>& structs = program->structs();

  EXPECT_EQ(structs.size(), 1);

  const t_struct* my_struct = structs[0];

  // Control case
  const t_field* first_field = my_struct->get_field_by_id(1);
  ASSERT_NE(first_field, nullptr);
  EXPECT_EQ(first_field->name(), "first");
  const t_type* first_field_type = first_field->get_type();
  ASSERT_NE(first_field_type, nullptr);
  EXPECT_NE(first_field_type->get_true_type(), nullptr);

  // Missing type case
  const t_field* second_field = my_struct->get_field_by_id(2);
  ASSERT_NE(second_field, nullptr);
  EXPECT_EQ(second_field->name(), "second");
  const t_type* second_field_type = second_field->get_type();
  ASSERT_NE(second_field_type, nullptr);
  EXPECT_EQ(second_field_type->get_true_type(), nullptr);
}
