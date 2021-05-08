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

#include <thrift/compiler/ast/ast_visitor.h>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/compiler/ast/t_function.h>
#include <thrift/compiler/ast/t_interaction.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_service.h>

namespace apache::thrift::compiler {
namespace {

std::unique_ptr<t_program> newTestProgram() {
  auto result = std::make_unique<t_program>("path/to/program.thrift");
  result->set_namespace("cpp2", "path.to");
  return result;
}

class MockVisitor {
 public:
  MOCK_METHOD(void, visit_program, (const t_program*));
  MOCK_METHOD(void, visit_service, (const t_service*));
  MOCK_METHOD(void, visit_interaction, (const t_interaction*));
  MOCK_METHOD(void, visit_function, (const t_function*));

  template <typename V>
  void addTo(V& visitor) {
    visitor.add_program_visitor(
        [this](const t_program* node) { visit_program(node); });
    visitor.add_service_visitor(
        [this](const t_service* node) { visit_service(node); });
    visitor.add_interaction_visitor(
        [this](const t_interaction* node) { visit_interaction(node); });
    visitor.add_function_visitor(
        [this](const t_function* node) { visit_function(node); });
  }
};

template <typename V>
class AstVisitorTest : public ::testing::Test {
 public:
  AstVisitorTest() noexcept { mock_visitor_.addTo(visitor_); }

  void visit(std::unique_ptr<t_program>& program) { visitor_(program.get()); }

 protected:
  ::testing::StrictMock<MockVisitor> mock_visitor_;

 private:
  V visitor_;
};

using AstVisitorTypes = ::testing::Types<ast_visitor, const_ast_visitor>;
TYPED_TEST_SUITE(AstVisitorTest, AstVisitorTypes);

TYPED_TEST(AstVisitorTest, Empty) {
  auto program = newTestProgram();
  EXPECT_CALL(this->mock_visitor_, visit_program(program.get()));
  this->visit(program);
}

TYPED_TEST(AstVisitorTest, All) {
  auto program = newTestProgram();
  EXPECT_CALL(this->mock_visitor_, visit_program(program.get()));

  auto interaction =
      std::make_unique<t_interaction>(program.get(), "Interaction");

  auto int_func1 = std::make_unique<t_function>(
      &t_base_type::t_void(),
      "int_func1",
      std::make_unique<t_paramlist>(program.get()));
  EXPECT_CALL(this->mock_visitor_, visit_function(int_func1.get()));
  interaction->add_function(std::move(int_func1));

  auto int_func2 = std::make_unique<t_function>(
      &t_base_type::t_void(),
      "int_func2",
      std::make_unique<t_paramlist>(program.get()));
  EXPECT_CALL(this->mock_visitor_, visit_function(int_func2.get()));
  interaction->add_function(std::move(int_func2));

  EXPECT_CALL(this->mock_visitor_, visit_interaction(interaction.get()));
  program->add_interaction(std::move(interaction));

  auto service = std::make_unique<t_service>(program.get(), "Service");
  EXPECT_CALL(this->mock_visitor_, visit_service(service.get()));
  program->add_service(std::move(service));

  this->visit(program);
}

TEST(AstVisitorTest, OverladedVisitor) {
  ast_visitor visitor;
  ::testing::StrictMock<MockVisitor> mock_visitor;
  struct {
    MockVisitor* mock_visitor_;
    void operator()(const t_interaction* ti) {
      mock_visitor_->visit_interaction(ti);
    }
    void operator()(const t_service* ts) { mock_visitor_->visit_service(ts); }
    void operator()(const t_program* tp) { mock_visitor_->visit_program(tp); }
  } overloaded_visitor{&mock_visitor};

  visitor.add_interface_visitor(overloaded_visitor);
  visitor.add_program_visitor(overloaded_visitor);

  auto program = newTestProgram();
  program->add_service(std::make_unique<t_service>(program.get(), "Service"));
  program->add_interaction(
      std::make_unique<t_interaction>(program.get(), "Interaction"));

  EXPECT_CALL(mock_visitor, visit_program(::testing::_));
  EXPECT_CALL(mock_visitor, visit_service(::testing::_));
  EXPECT_CALL(mock_visitor, visit_interaction(::testing::_));
  visitor(program.get());
}

} // namespace
} // namespace apache::thrift::compiler
