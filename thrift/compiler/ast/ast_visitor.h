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

#include <thrift/compiler/ast/t_interaction.h>
#include <thrift/compiler/ast/t_interface.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_service.h>

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
  using program_type = node_type<t_program>;
  using service_type = node_type<t_service>;
  using interaction_type = node_type<t_interaction>;
  using function_type = node_type<t_function>;

 public:
  ////
  // Visitation entry points.
  ////

  void operator()(program_type* node) const;
  void operator()(service_type* node) const;
  void operator()(interaction_type* node) const;
  void operator()(function_type* node) const;

  ////
  // Visitor registration methods.
  ////

  // Whole program visitor.
  void add_program_visitor(std::function<void(program_type*)> visitor) {
    program_visitors_.emplace_back(std::move(visitor));
  }

  // Interface visitors.
  void add_service_visitor(std::function<void(service_type*)> visitor) {
    service_visitors_.emplace_back(std::move(visitor));
  }
  void add_interaction_visitor(std::function<void(interaction_type*)> visitor) {
    interaction_visitors_.emplace_back(std::move(visitor));
  }

  // Function visitors.
  void add_function_visitor(std::function<void(function_type*)> visitor) {
    function_visitors_.emplace_back(std::move(visitor));
  }

  // Helper functions for visiting multiple types of nodes.
  template <typename V>
  void add_interface_visitor(V&& visitor) {
    add_service_visitor(visitor);
    add_interaction_visitor(std::forward<V>(visitor));
  }

 private:
  visitor_list<program_type*> program_visitors_;
  visitor_list<service_type*> service_visitors_;
  visitor_list<interaction_type*> interaction_visitors_;
  visitor_list<function_type*> function_visitors_;
};

} // namespace detail

using ast_visitor = detail::ast_visitor<false>;
using const_ast_visitor = detail::ast_visitor<true>;

} // namespace compiler
} // namespace thrift
} // namespace apache
