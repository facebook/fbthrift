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

namespace apache {
namespace thrift {
namespace compiler {
namespace detail {

namespace {

template <typename... Args>
void visit(const visitor_list<Args...>& visitors, const Args&... args) {
  for (const auto& visitor : visitors) {
    visitor(args...);
  }
}

template <typename V, typename C, typename... Args>
void visit_children(V& visitor, const C& children, const Args&... args) {
  for (auto* child : children) {
    visitor(args..., child);
  }
}

} // namespace

template <bool is_const>
void ast_visitor<is_const>::operator()(program_type* node) const {
  visit(program_visitors_, node);
  visit_children(*this, node->services());
  visit_children(*this, node->interactions());
  // TODO(afuller): Split structs and unions in t_program accessors.
  for (auto* struct_or_union : node->structs()) {
    if (auto* tunion = dynamic_cast<union_type*>(struct_or_union)) {
      this->operator()(tunion);
    } else {
      this->operator()(struct_or_union);
    }
  }
  visit_children(*this, node->exceptions());
  visit_children(*this, node->typedefs());
  visit_children(*this, node->enums());
  visit_children(*this, node->consts());
}

template <bool is_const>
void ast_visitor<is_const>::operator()(service_type* node) const {
  assert(typeid(*node) == typeid(service_type)); // Must actually be a service.
  visit(service_visitors_, node);
  visit_children(*this, node->functions());
}
template <bool is_const>
void ast_visitor<is_const>::operator()(interaction_type* node) const {
  visit(interaction_visitors_, node);
  visit_children(*this, node->functions());
}
template <bool is_const>
void ast_visitor<is_const>::operator()(function_type* node) const {
  visit(function_visitors_, node);
}

template <bool is_const>
void ast_visitor<is_const>::operator()(struct_type* node) const {
  assert(typeid(*node) == typeid(struct_type)); // Must actually be a struct.
  visit(struct_visitors_, node);
  visit_children(*this, node->get_members());
}
template <bool is_const>
void ast_visitor<is_const>::operator()(union_type* node) const {
  visit(union_visitors_, node);
  visit_children(*this, node->get_members());
}
template <bool is_const>
void ast_visitor<is_const>::operator()(exception_type* node) const {
  visit(exception_visitors_, node);
  visit_children(*this, node->get_members());
}
template <bool is_const>
void ast_visitor<is_const>::operator()(field_type* node) const {
  visit(field_visitors_, node);
}

template <bool is_const>
void ast_visitor<is_const>::operator()(enum_type* node) const {
  visit(enum_visitors_, node);
  visit_children(*this, node->enum_values());
}
template <bool is_const>
void ast_visitor<is_const>::operator()(enum_value_type* node) const {
  visit(enum_value_visitors_, node);
}
template <bool is_const>
void ast_visitor<is_const>::operator()(const_type* node) const {
  visit(const_visitors_, node);
}

template <bool is_const>
void ast_visitor<is_const>::operator()(typedef_type* node) const {
  visit(typedef_visitors_, node);
}

template class ast_visitor<true>;
template class ast_visitor<false>;

} // namespace detail
} // namespace compiler
} // namespace thrift
} // namespace apache
