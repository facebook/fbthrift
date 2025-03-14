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

#include <thrift/compiler/whisker/detail/overload.h>
#include <thrift/compiler/whisker/print_ast.h>
#include <thrift/compiler/whisker/tree_printer.h>

#include <ostream>

#include <fmt/core.h>

namespace whisker {

namespace {

std::string to_string(
    const source_range& loc, const source_manager& src_manager) {
  resolved_location begin(loc.begin, src_manager);
  resolved_location end(loc.end, src_manager);
  // Format of a source location is:
  //   line:<line>:<column>
  // If begin and end are on the same line, the the end source location is:
  //   col:<column>
  return fmt::format(
      "<line:{}:{}, {}>",
      begin.line(),
      begin.column(),
      begin.line() == end.line()
          ? fmt::format("col:{}", end.column())
          : fmt::format("line:{}:{}", end.line(), end.column()));
}

struct ast_visitor {
  std::string location(const source_range& loc) const {
    return to_string(loc, src_manager);
  }

  void visit(const ast::identifier& id, tree_printer::scope scope) const {
    scope.println(" identifier '{}'", id.name);
  }
  void visit(const ast::text& text, tree_printer::scope scope) const {
    scope.println(
        " text {} '{}'",
        location(text.loc),
        tree_printer::escape(text.joined()));
  }
  void visit(const ast::newline& newline, tree_printer::scope scope) const {
    scope.println(
        " newline {} '{}'",
        location(newline.loc),
        tree_printer::escape(newline.text));
  }
  void visit(
      const ast::section_block& section, tree_printer::scope scope) const {
    scope.println(
        " section-block{} {}",
        section.inverted ? " <inverted>" : "",
        location(section.loc));
    visit(section.variable, scope.open_property());
    for (const auto& body : section.body_elements) {
      visit(body, scope.open_node());
    }
  }
  void visit(
      const ast::conditional_block& conditional_block,
      tree_printer::scope scope) const {
    scope.println(" if-block {}", location(conditional_block.loc));
    visit(conditional_block.condition, scope.open_property());
    visit(conditional_block.body_elements, scope.open_node());

    for (const auto& else_if_clause : conditional_block.else_if_clauses) {
      auto else_if_scope = scope.open_property();
      else_if_scope.println(" else-if-block {}", location(else_if_clause.loc));
      visit(else_if_clause.body_elements, else_if_scope.open_node());
    }

    if (const auto& else_clause = conditional_block.else_clause) {
      auto else_scope = scope.open_property();
      else_scope.println(" else-block {}", location(else_clause->loc));
      visit(else_clause->body_elements, else_scope.open_node());
    }
  }
  void visit(
      const ast::with_block& with_block, tree_printer::scope scope) const {
    scope.println(" with-block {}", location(with_block.loc));
    visit(with_block.value, scope.open_property());
    visit(with_block.body_elements, scope.open_node());
  }
  void visit(
      const ast::each_block& each_block, tree_printer::scope scope) const {
    scope.println(" each-block {}", location(each_block.loc));
    visit(each_block.iterable, scope.open_property());
    if (const auto& captured = each_block.captured) {
      scope.open_property().println(
          " element-capture '{}'", captured->element.name);
      if (const auto& index = captured->index) {
        scope.open_property().println(" index-capture '{}'", index->name);
      }
    }
    visit(each_block.body_elements, scope.open_node());
    if (const auto& else_clause = each_block.else_clause) {
      auto else_scope = scope.open_property();
      else_scope.println(" else-block {}", location(else_clause->loc));
      visit(else_clause->body_elements, else_scope.open_node());
    }
  }
  void visit(const ast::macro& macro, tree_printer::scope scope) const {
    scope.println(" macro {} '{}'", location(macro.loc), macro.path_string());
    if (const auto& indentation = macro.standalone_indentation_within_line;
        indentation.has_value()) {
      scope.open_property().println(
          " standalone-indentation '{}'",
          tree_printer::escape(indentation->value));
    }
  }
  void visit(const ast::comment& comment, tree_printer::scope scope) const {
    scope.println(
        " comment {} '{}'",
        location(comment.loc),
        tree_printer::escape(comment.text));
  }
  void visit(
      const ast::variable_lookup& variable, tree_printer::scope scope) const {
    scope.println(
        " variable-lookup {} '{}'",
        location(variable.loc),
        variable.chain_string());
  }
  void visit(const ast::expression& expr, tree_printer::scope scope) const {
    scope.println(" expression {} '{}'", location(expr.loc), expr.to_string());
  }
  void visit(const ast::interpolation& interpolation, tree_printer::scope scope)
      const {
    scope.println(
        " interpolation {} '{}'",
        location(interpolation.loc),
        interpolation.to_string());
  }
  void visit(const ast::let_statement& let_statement, tree_printer::scope scope)
      const {
    scope.println(" let-statement {}", location(let_statement.loc));
    if (let_statement.exported) {
      scope.open_property().println(" exported");
    }
    visit(let_statement.id, scope.open_property());
    visit(let_statement.value, scope.open_property());
  }
  void visit(
      const ast::pragma_statement& pragma_statement,
      tree_printer::scope scope) const {
    scope.println(
        " pragma-statement '{}' {}",
        pragma_statement.to_string(),
        location(pragma_statement.loc));
  }
  void visit(const ast::partial_block& partial_block, tree_printer::scope scope)
      const {
    scope.println(
        " partial-block {} '{}'",
        location(partial_block.loc),
        partial_block.name.name);
    if (partial_block.exported) {
      scope.open_property().println(" exported");
    }
    for (const auto& argument : partial_block.arguments) {
      scope.open_property().println(" argument '{}'", argument.name);
    }
    for (const auto& capture : partial_block.captures) {
      scope.open_property().println(" capture '{}'", capture.name);
    }
    visit(partial_block.body_elements, scope.open_node());
  }
  void visit(
      const ast::partial_statement& partial_statement,
      tree_printer::scope scope) const {
    scope.println(
        " partial-statement {} '{}'",
        location(partial_statement.loc),
        partial_statement.partial.to_string());
    if (const auto& indentation =
            partial_statement.standalone_indentation_within_line;
        indentation.has_value()) {
      scope.open_property().println(
          " standalone-indentation '{}'",
          tree_printer::escape(indentation->value));
    }
    for (const auto& [name, arg] : partial_statement.named_arguments) {
      scope.open_property().println(
          " argument '{}={}'", name, arg.value.to_string());
    }
  }
  void visit(
      const ast::import_statement& import_statement,
      tree_printer::scope scope) const {
    scope.println(" import-statement {}", location(import_statement.loc));
    scope.open_property().println(" path '{}'", import_statement.path.text);
    scope.open_property().println(" name '{}'", import_statement.name.name);
  }

  void visit(
      const ast::headers& header_elements, tree_printer::scope scope) const {
    scope.println(" header");
    auto header_scope = scope.open_node();
    for (const auto& header : header_elements) {
      visit(header, header_scope);
    }
  }
  void visit(
      const ast::bodies& body_elements, tree_printer::scope scope) const {
    // This node is transparent so it does not directly appear in the tree
    for (const auto& body : body_elements) {
      visit(body, scope);
    }
  }
  // Prevent implicit conversion to ast::header or ast::body. Otherwise, we can
  // silently compile an infinitely recursive visit() chain if there is a
  // missing overload for one of the alternatives in the variant.
  template <
      class T,
      std::enable_if_t<
          std::is_same_v<T, ast::header> || std::is_same_v<T, ast::body>>* =
          nullptr>
  void visit(const T& elements, tree_printer::scope scope) const {
    // This node is transparent so it does not directly appear in the tree
    detail::variant_match(
        elements, [&](const auto& node) { visit(node, std::move(scope)); });
  }

  void visit(const ast::root& root, tree_printer::scope scope) const {
    scope.println(
        "root [{}]", resolved_location(root.loc, src_manager).file_name());
    auto root_scope = scope.open_node();
    if (!root.header_elements.empty()) {
      visit(root.header_elements, root_scope);
    }
    visit(root.body_elements, root_scope);
  }

  const source_manager& src_manager;
};

} // namespace

template <class Node>
void print_ast(
    const Node& root_node,
    const source_manager& src_manager,
    std::ostream& out) {
  ast_visitor{src_manager}.visit(
      root_node, tree_printer::scope::make_root(out));
}

#define WHISKER_PRINT_AST_INSTANTIATE(node_type) \
  template void print_ast(                       \
      const node_type&, const source_manager&, std::ostream&)

WHISKER_PRINT_AST_INSTANTIATE(ast::root);
WHISKER_PRINT_AST_INSTANTIATE(ast::text);
WHISKER_PRINT_AST_INSTANTIATE(ast::comment);
WHISKER_PRINT_AST_INSTANTIATE(ast::identifier);
WHISKER_PRINT_AST_INSTANTIATE(ast::variable_lookup);
WHISKER_PRINT_AST_INSTANTIATE(ast::section_block);
WHISKER_PRINT_AST_INSTANTIATE(ast::macro);

#undef WHISKER_PRINT_AST_INSTANTIATE

} // namespace whisker
