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

#pragma once

#include <functional>
#include <type_traits>
#include <vector>

#include <thrift/compiler/ast/t_const.h>
#include <thrift/compiler/ast/t_enum.h>
#include <thrift/compiler/ast/t_enum_value.h>
#include <thrift/compiler/ast/t_exception.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_function.h>
#include <thrift/compiler/ast/t_interaction.h>
#include <thrift/compiler/ast/t_interface.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_typedef.h>
#include <thrift/compiler/ast/t_union.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace detail {

// A list of visitor that accept the given arguments.
template <typename... Args>
using visitor_list = std::vector<std::function<void(Args...)>>;

// A class that can traverse ast nodes, invoking registered visitors for each
// node visited.
//
// Visits AST nodes in 'preorder', visiting the parent node before children
// nodes.
template <bool is_const>
class ast_visitor {
  // The type to use when traversing the given node type N.
  template <typename N>
  using node_type = std::conditional_t<is_const, const N, N>;

 public:
// Visitation and registration functions for concrete AST nodes.
//
// For each node type, provides the following functions:
// - an operator() overload for visiting the node:
//     void operator()(t_{name}*) const;
// - a function to add a node-specific visitor:
//     void add_{name}_visitor(std::function<void(t_{name}*)>);
#define __FBTHRIFT_AST_VISITOR_NODE_T_(name)                             \
  using name##_type = node_type<t_##name>;                               \
  void operator()(name##_type* node) const;                              \
  void add_##name##_visitor(std::function<void(name##_type*)> visitor) { \
    name##_visitors_.emplace_back(std::move(visitor));                   \
  }
  __FBTHRIFT_AST_VISITOR_NODE_T_(program)
  __FBTHRIFT_AST_VISITOR_NODE_T_(service)
  __FBTHRIFT_AST_VISITOR_NODE_T_(interaction)
  __FBTHRIFT_AST_VISITOR_NODE_T_(function)

  __FBTHRIFT_AST_VISITOR_NODE_T_(struct)
  __FBTHRIFT_AST_VISITOR_NODE_T_(union)
  __FBTHRIFT_AST_VISITOR_NODE_T_(exception)
  __FBTHRIFT_AST_VISITOR_NODE_T_(field)

  __FBTHRIFT_AST_VISITOR_NODE_T_(enum)
  __FBTHRIFT_AST_VISITOR_NODE_T_(enum_value)
  __FBTHRIFT_AST_VISITOR_NODE_T_(const)

  __FBTHRIFT_AST_VISITOR_NODE_T_(typedef)
#undef __FBTHRIFT_AST_VISITOR_NODE_T_

  // Adds visitor for all interface node types.
  //
  // For example: t_service and t_interaction.
  template <typename V>
  void add_interface_visitor(V&& visitor) {
    add_service_visitor(visitor);
    add_interaction_visitor(std::forward<V>(visitor));
  }

  // Adds a visitor for all structured IDL definition node types.
  //
  // For example: t_struct, t_union, and t_exception.
  // Does not include other t_structured nodes like t_paramlist.
  template <typename V>
  void add_structured_definition_visitor(V&& visitor) {
    add_struct_visitor(visitor);
    add_union_visitor(visitor);
    add_exception_visitor(std::forward<V>(visitor));
  }

  // Adds a visitor for all IDL definition node types.
  template <typename V>
  void add_definition_visitor(V&& visitor) {
    add_interface_visitor(visitor);
    add_function_visitor(visitor);

    add_structured_definition_visitor(visitor);
    add_field_visitor(visitor);

    add_enum_visitor(visitor);
    add_enum_value_visitor(visitor);
    add_const_visitor(visitor);

    add_typedef_visitor(std::forward<V>(visitor));
  }

 private:
  visitor_list<program_type*> program_visitors_;
  visitor_list<service_type*> service_visitors_;
  visitor_list<interaction_type*> interaction_visitors_;
  visitor_list<function_type*> function_visitors_;

  visitor_list<struct_type*> struct_visitors_;
  visitor_list<union_type*> union_visitors_;
  visitor_list<exception_type*> exception_visitors_;
  visitor_list<field_type*> field_visitors_;

  visitor_list<enum_type*> enum_visitors_;
  visitor_list<enum_value_type*> enum_value_visitors_;
  visitor_list<const_type*> const_visitors_;

  visitor_list<typedef_type*> typedef_visitors_;
};

} // namespace detail

using ast_visitor = detail::ast_visitor<false>;
using const_ast_visitor = detail::ast_visitor<true>;

} // namespace compiler
} // namespace thrift
} // namespace apache
