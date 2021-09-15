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

#include <memory>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache::thrift::compiler {
namespace {

// A helper class for keeping track of visitation expectations.
class MockAstVisitor {
 public:
  MOCK_METHOD(void, visit_program, (const t_program*));
  MOCK_METHOD(void, visit_service, (const t_service*));
  MOCK_METHOD(void, visit_interaction, (const t_interaction*));
  MOCK_METHOD(void, visit_function, (const t_function*));

  MOCK_METHOD(void, visit_struct, (const t_struct*));
  MOCK_METHOD(void, visit_union, (const t_union*));
  MOCK_METHOD(void, visit_exception, (const t_exception*));
  MOCK_METHOD(void, visit_field, (const t_field*));

  MOCK_METHOD(void, visit_enum, (const t_enum*));
  MOCK_METHOD(void, visit_enum_value, (const t_enum_value*));
  MOCK_METHOD(void, visit_const, (const t_const*));

  MOCK_METHOD(void, visit_typedef, (const t_typedef*));

  MOCK_METHOD(void, visit_interface, (const t_interface*));
  MOCK_METHOD(void, visit_structured_definition, (const t_structured*));
  MOCK_METHOD(void, visit_definition, (const t_named*));

  // Registers with all ast_visitor registration functions.
  template <typename V>
  void addTo(V& visitor) {
    visitor.add_program_visitor(
        [this](const t_program& node) { visit_program(&node); });
    visitor.add_service_visitor(
        [this](const t_service& node) { visit_service(&node); });
    visitor.add_interaction_visitor(
        [this](const t_interaction& node) { visit_interaction(&node); });
    visitor.add_function_visitor(
        [this](const t_function& node) { visit_function(&node); });

    visitor.add_struct_visitor(
        [this](const t_struct& node) { visit_struct(&node); });
    visitor.add_union_visitor(
        [this](const t_union& node) { visit_union(&node); });
    visitor.add_exception_visitor(
        [this](const t_exception& node) { visit_exception(&node); });
    visitor.add_field_visitor(
        [this](const t_field& node) { visit_field(&node); });

    visitor.add_enum_visitor([this](const t_enum& node) { visit_enum(&node); });
    visitor.add_enum_value_visitor(
        [this](const t_enum_value& node) { visit_enum_value(&node); });
    visitor.add_const_visitor(
        [this](const t_const& node) { visit_const(&node); });

    visitor.add_typedef_visitor(
        [this](const t_typedef& node) { visit_typedef(&node); });

    visitor.add_interface_visitor(
        [this](const t_interface& node) { visit_interface(&node); });
    visitor.add_structured_definition_visitor([this](const t_structured& node) {
      visit_structured_definition(&node);
    });
    visitor.add_definition_visitor(
        [this](const t_named& node) { visit_definition(&node); });
  }
};

// A visitor for all node types, that forwards to the type-specific
// MockAstVisitor functions.
class OverloadedVisitor {
 public:
  explicit OverloadedVisitor(MockAstVisitor* mock) : mock_(mock) {}

  void operator()(const t_interaction& node) {
    mock_->visit_interaction(&node);
  }
  void operator()(const t_service& node) { mock_->visit_service(&node); }
  void operator()(const t_program& node) { mock_->visit_program(&node); }
  void operator()(const t_function& node) { mock_->visit_function(&node); }

  void operator()(const t_struct& node) { mock_->visit_struct(&node); }
  void operator()(const t_union& node) { mock_->visit_union(&node); }
  void operator()(const t_exception& node) { mock_->visit_exception(&node); }
  void operator()(const t_field& node) { mock_->visit_field(&node); }

  void operator()(const t_enum& node) { mock_->visit_enum(&node); }
  void operator()(const t_enum_value& node) { mock_->visit_enum_value(&node); }
  void operator()(const t_const& node) { mock_->visit_const(&node); }

  void operator()(const t_typedef& node) { mock_->visit_typedef(&node); }

 private:
  MockAstVisitor* mock_;
};

template <typename V>
class AstVisitorTest : public ::testing::Test {
 public:
  AstVisitorTest() noexcept
      : program_("path/to/program.thrift"),
        overload_visitor_(&overload_mock_) {}

  void SetUp() override {
    // Register mock_ to verify add_* function -> nodes visited
    // relationship.
    mock_.addTo(visitor_);
    // Register overload_visitor_ with each multi-node visitor function to
    // verify the correct overloads are used.
    visitor_.add_interface_visitor(overload_visitor_);
    visitor_.add_structured_definition_visitor(overload_visitor_);
    visitor_.add_definition_visitor(overload_visitor_);

    // Add baseline expectations.
    EXPECT_CALL(this->mock_, visit_program(&this->program_));
  }

  void TearDown() override { visitor_(program_); }

 protected:
  ::testing::StrictMock<MockAstVisitor> mock_;
  ::testing::StrictMock<MockAstVisitor> overload_mock_;
  t_program program_;
  t_scope scope_;

 private:
  V visitor_;
  OverloadedVisitor overload_visitor_;
};

using AstVisitorTypes = ::testing::Types<ast_visitor, const_ast_visitor>;
TYPED_TEST_SUITE(AstVisitorTest, AstVisitorTypes);

TYPED_TEST(AstVisitorTest, EmptyProgram) {}

TYPED_TEST(AstVisitorTest, Interaction) {
  auto interaction =
      std::make_unique<t_interaction>(&this->program_, "Interaction");
  auto func = std::make_unique<t_function>(
      &t_base_type::t_void(),
      "function",
      std::make_unique<t_paramlist>(&this->program_));

  EXPECT_CALL(this->mock_, visit_function(func.get()));
  // Matches: definition.
  EXPECT_CALL(this->mock_, visit_definition(func.get()));
  EXPECT_CALL(this->overload_mock_, visit_function(func.get()));
  interaction->add_function(std::move(func));

  EXPECT_CALL(this->mock_, visit_interaction(interaction.get()));
  // Matches: interface, definition.
  EXPECT_CALL(this->mock_, visit_interface(interaction.get()));
  EXPECT_CALL(this->mock_, visit_definition(interaction.get()));
  EXPECT_CALL(this->overload_mock_, visit_interaction(interaction.get()))
      .Times(2);
  this->program_.add_interaction(std::move(interaction));
}

TYPED_TEST(AstVisitorTest, Service) {
  auto service = std::make_unique<t_service>(&this->program_, "Service");
  auto func = std::make_unique<t_function>(
      &t_base_type::t_void(),
      "function",
      std::make_unique<t_paramlist>(&this->program_));

  EXPECT_CALL(this->mock_, visit_function(func.get()));
  // Matches: definition.
  EXPECT_CALL(this->mock_, visit_definition(func.get()));
  EXPECT_CALL(this->overload_mock_, visit_function(func.get()));
  service->add_function(std::move(func));

  EXPECT_CALL(this->mock_, visit_service(service.get()));
  // Matches: interface, definition.
  EXPECT_CALL(this->mock_, visit_interface(service.get()));
  EXPECT_CALL(this->mock_, visit_definition(service.get()));
  EXPECT_CALL(this->overload_mock_, visit_service(service.get())).Times(2);
  this->program_.add_service(std::move(service));
}

TYPED_TEST(AstVisitorTest, Struct) {
  auto tstruct = std::make_unique<t_struct>(&this->program_, "Struct");
  auto field =
      std::make_unique<t_field>(&t_base_type::t_i32(), "struct_field", 1);
  EXPECT_CALL(this->mock_, visit_field(field.get()));
  // Matches: definition.
  EXPECT_CALL(this->mock_, visit_definition(field.get()));
  EXPECT_CALL(this->overload_mock_, visit_field(field.get()));
  tstruct->append(std::move(field));

  EXPECT_CALL(this->mock_, visit_struct(tstruct.get()));
  // Matches: structured_definition, definition.
  EXPECT_CALL(this->mock_, visit_structured_definition(tstruct.get()));
  EXPECT_CALL(this->mock_, visit_definition(tstruct.get()));
  EXPECT_CALL(this->overload_mock_, visit_struct(tstruct.get())).Times(2);
  this->program_.add_struct(std::move(tstruct));
}

TYPED_TEST(AstVisitorTest, Union) {
  auto tunion = std::make_unique<t_union>(&this->program_, "Union");
  auto field =
      std::make_unique<t_field>(&t_base_type::t_i32(), "union_field", 1);
  EXPECT_CALL(this->mock_, visit_field(field.get()));
  // Matches: definition.
  EXPECT_CALL(this->mock_, visit_definition(field.get()));
  EXPECT_CALL(this->overload_mock_, visit_field(field.get()));
  tunion->append(std::move(field));

  EXPECT_CALL(this->mock_, visit_union(tunion.get()));
  // Matches: structured_definition, definition.
  EXPECT_CALL(this->mock_, visit_structured_definition(tunion.get()));
  EXPECT_CALL(this->mock_, visit_definition(tunion.get()));
  EXPECT_CALL(this->overload_mock_, visit_union(tunion.get())).Times(2);
  this->program_.add_struct(std::move(tunion));
}

TYPED_TEST(AstVisitorTest, Exception) {
  auto except = std::make_unique<t_exception>(&this->program_, "Exception");
  auto field =
      std::make_unique<t_field>(&t_base_type::t_i32(), "exception_field", 1);
  EXPECT_CALL(this->mock_, visit_field(field.get()));
  // Matches: definition.
  EXPECT_CALL(this->mock_, visit_definition(field.get()));
  EXPECT_CALL(this->overload_mock_, visit_field(field.get()));
  except->append(std::move(field));

  EXPECT_CALL(this->mock_, visit_exception(except.get()));
  // Matches: structured_definition, definition.
  EXPECT_CALL(this->mock_, visit_structured_definition(except.get()));
  EXPECT_CALL(this->mock_, visit_definition(except.get()));
  EXPECT_CALL(this->overload_mock_, visit_exception(except.get())).Times(2);
  this->program_.add_exception(std::move(except));
}

TYPED_TEST(AstVisitorTest, Enum) {
  auto tenum = std::make_unique<t_enum>(&this->program_, "Enum");
  auto tenum_value = std::make_unique<t_enum_value>("EnumValue");
  EXPECT_CALL(this->mock_, visit_enum_value(tenum_value.get()));
  // Matches: definition.
  EXPECT_CALL(this->mock_, visit_definition(tenum_value.get()));
  EXPECT_CALL(this->overload_mock_, visit_enum_value(tenum_value.get()));
  tenum->append(std::move(tenum_value));

  EXPECT_CALL(this->mock_, visit_enum(tenum.get()));
  // Matches: definition.
  EXPECT_CALL(this->mock_, visit_definition(tenum.get()));
  EXPECT_CALL(this->overload_mock_, visit_enum(tenum.get()));
  this->program_.add_enum(std::move(tenum));
}

TYPED_TEST(AstVisitorTest, Const) {
  auto tconst = std::make_unique<t_const>(
      &this->program_, &t_base_type::t_i32(), "Const", nullptr);
  EXPECT_CALL(this->mock_, visit_const(tconst.get()));
  // Matches: definition.
  EXPECT_CALL(this->mock_, visit_definition(tconst.get()));
  EXPECT_CALL(this->overload_mock_, visit_const(tconst.get()));
  this->program_.add_const(std::move(tconst));
}

TYPED_TEST(AstVisitorTest, Typedef) {
  auto ttypedef = std::make_unique<t_typedef>(
      &this->program_, &t_base_type::t_i32(), "Typedef", nullptr);
  EXPECT_CALL(this->mock_, visit_typedef(ttypedef.get()));
  // Matches: definition.
  EXPECT_CALL(this->mock_, visit_definition(ttypedef.get()));
  EXPECT_CALL(this->overload_mock_, visit_typedef(ttypedef.get()));
  this->program_.add_typedef(std::move(ttypedef));
}

class MockObserver {
 public:
  MOCK_METHOD(void, begin_visit, (t_node&));
  MOCK_METHOD(void, end_visit, (t_node&));
};

TEST(ObserverTest, OrderOfCalls) {
  static_assert(ast_detail::is_observer<MockObserver&>::value, "");
  using test_ast_visitor =
      basic_ast_visitor<false, MockObserver&, int, MockObserver&>;

  t_program program("path/to/program.thrift");
  auto* tunion = new t_union(&program, "Union");
  auto* field = new t_field(&t_base_type::t_i32(), "union_field", 1);
  tunion->append(std::unique_ptr<t_field>(field));
  program.add_struct(std::unique_ptr<t_union>(tunion));

  MockObserver m1, m2;
  {
    ::testing::InSequence ins;
    EXPECT_CALL(m1, begin_visit(::testing::Ref(program)));
    EXPECT_CALL(m2, begin_visit(::testing::Ref(program)));
    EXPECT_CALL(m1, begin_visit(::testing::Ref(*tunion)));
    EXPECT_CALL(m2, begin_visit(::testing::Ref(*tunion)));
    EXPECT_CALL(m1, begin_visit(::testing::Ref(*field)));
    EXPECT_CALL(m2, begin_visit(::testing::Ref(*field)));

    // End is called in reverse order.
    EXPECT_CALL(m2, end_visit(::testing::Ref(*field)));
    EXPECT_CALL(m1, end_visit(::testing::Ref(*field)));
    EXPECT_CALL(m2, end_visit(::testing::Ref(*tunion)));
    EXPECT_CALL(m1, end_visit(::testing::Ref(*tunion)));
    EXPECT_CALL(m2, end_visit(::testing::Ref(program)));
    EXPECT_CALL(m1, end_visit(::testing::Ref(program)));
  }
  test_ast_visitor visitor;
  visitor(m1, 1, m2, program);
}

TEST(ObserverTest, VisitContext) {
  static_assert(ast_detail::is_observer<visitor_context&>::value, "");
  using ctx_ast_visitor = basic_ast_visitor<false, visitor_context&>;

  t_program program("path/to/program.thrift");
  auto* tunion = new t_union(&program, "Union");
  auto* field = new t_field(&t_base_type::t_i32(), "union_field", 1);
  tunion->append(std::unique_ptr<t_field>(field));
  program.add_struct(std::unique_ptr<t_union>(tunion));

  int calls = 0;
  ctx_ast_visitor visitor;
  visitor.add_program_visitor([&](visitor_context& ctx, t_program& node) {
    EXPECT_EQ(&node, &program);
    EXPECT_EQ(ctx.parent(), nullptr);
    ++calls;
  });
  visitor.add_union_visitor([&](visitor_context& ctx, t_union& node) {
    EXPECT_EQ(&node, tunion);
    EXPECT_EQ(ctx.parent(), &program);
    ++calls;
  });
  visitor.add_field_visitor([&](visitor_context& ctx, t_field& node) {
    EXPECT_EQ(&node, field);
    EXPECT_EQ(ctx.parent(), tunion);
    ++calls;
  });

  visitor_context ctx;
  EXPECT_EQ(ctx.parent(), nullptr);
  visitor(ctx, program);
  EXPECT_EQ(calls, 3);
}

} // namespace
} // namespace apache::thrift::compiler
