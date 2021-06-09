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

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <thrift/compiler/ast/t_enum.h>
#include <thrift/compiler/ast/t_enum_value.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_interface.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/lib/cpp2/util.h>
#include <thrift/compiler/sema/scope_validator.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

struct service_metadata {
  std::unordered_map<std::string, const t_service*> function_name_to_service;

  service_metadata(node_metadata_cache& cache, const t_service* node) {
    if (node->extends() != nullptr) {
      function_name_to_service =
          cache.get<service_metadata>(node->extends()).function_name_to_service;
    }
    for (const auto* function : node->functions()) {
      function_name_to_service[function->name()] = node;
    }
  }
};

void validate_interface_function_name_uniqueness(
    diagnostic_context& ctx, const t_interface* node) {
  // Check for a redefinition of a function in the same interface.
  std::unordered_set<std::string> seen;
  for (const auto* function : node->functions()) {
    if (!seen.emplace(function->name()).second) {
      ctx.failure(
          function,
          "Function `%s` is already defined in `%s`.",
          function->name().c_str(),
          node->name().c_str());
    }
  }
}

void validate_extends_service_function_name_uniqueness(
    diagnostic_context& ctx, const t_service* node) {
  if (node->extends() == nullptr) {
    return;
  }

  const auto& extends_metadata =
      ctx.cache().get<service_metadata>(node->extends());
  for (const auto* function : node->functions()) {
    auto itr = extends_metadata.function_name_to_service.find(function->name());
    if (itr != extends_metadata.function_name_to_service.end()) {
      ctx.failure(
          function,
          "Function `%s.%s` redefines `%s.%s`.",
          node->name().c_str(),
          function->name().c_str(),
          itr->second->get_full_name().c_str(),
          itr->first.c_str());
    }
  }
}

void validate_union_field_attributes(
    diagnostic_context& ctx, const t_union* node) {
  for (const auto* field : node->fields()) {
    if (field->qualifier() != t_field_qualifier::unspecified) {
      ctx.failure(
          field,
          "Unions cannot contain qualified fields. Remove `%s` qualifier from field `%s`.",
          field->qualifier() == t_field_qualifier::required ? "required"
                                                            : "optional",
          field->name().c_str());
    }
    if (cpp2::is_mixin(*field)) {
      ctx.failure(
          field,
          "Union `%s` cannot contain mixin field `%s`.",
          node->name().c_str(),
          field->name().c_str());
    }
  }
}

void validate_mixin_field_attributes(
    diagnostic_context& ctx, const t_field* field) {
  if (!cpp2::is_mixin(*field)) {
    return;
  }

  auto* ttype = field->type()->get_true_type();
  if (typeid(*ttype) != typeid(t_struct) && typeid(*ttype) != typeid(t_union)) {
    ctx.failure(
        field,
        "Mixin field `%s` type must be a struct or union. Found `%s`.",
        field->name().c_str(),
        ttype->get_name().c_str());
  }

  if (field->qualifier() == t_field_qualifier::optional) {
    // Nothing technically stops us from marking optional field mixin.
    // However, this will bring surprising behavior. e.g. `foo.bar_ref()`
    // might throw `bad_field_access` if `bar` is inside optional mixin
    // field.
    ctx.failure(
        field, "Mixin field `%s` cannot be optional.", field->name().c_str());
  }
}

void validate_enum_value_name_uniqueness(
    diagnostic_context& ctx, const t_enum* node) {
  std::unordered_set<std::string> names;
  for (const auto* value : node->values()) {
    if (!names.insert(value->name()).second) {
      ctx.failure(
          value,
          "Redefinition of value `%s` in enum `%s`.",
          value->name().c_str(),
          node->name().c_str());
    }
  }
}

void validate_enum_value_uniqueness(
    diagnostic_context& ctx, const t_enum* node) {
  std::unordered_map<int32_t, const t_enum_value*> values;
  for (const auto* value : node->values()) {
    auto prev = values.emplace(value->get_value(), value);
    if (!prev.second) {
      ctx.failure(
          value,
          "Duplicate value `%s=%d` with value `%s` in enum `%s`.",
          value->name().c_str(),
          value->get_value(),
          prev.first->second->name().c_str(),
          node->name().c_str());
    }
  }
}

void validate_enum_value_explicit(
    diagnostic_context& ctx, const t_enum_value* node) {
  if (!node->has_value()) {
    ctx.failure(
        node,
        "The enum value, `%s`, must have an explicitly assigned value.",
        node->name().c_str());
  }
}

void validate_structured_annotation_type_uniqueness(
    diagnostic_context& ctx, const t_named* node) {
  std::unordered_set<const t_type*> seen;
  for (const auto& annot : node->structured_annotations()) {
    if (!seen.emplace(&annot->type().deref()).second) {
      ctx.failure(
          annot,
          "Duplicate structured annotation `%s` on `%s`.",
          annot->type()->name().c_str(),
          node->name().c_str());
    }
  }
}

} // namespace

ast_validator standard_validator() {
  ast_validator validator;
  validator.add_interface_visitor(&validate_interface_function_name_uniqueness);
  validator.add_service_visitor(
      &validate_extends_service_function_name_uniqueness);

  validator.add_union_visitor(&validate_union_field_attributes);
  validator.add_field_visitor(&validate_mixin_field_attributes);

  validator.add_enum_visitor(&validate_enum_value_name_uniqueness);
  validator.add_enum_visitor(&validate_enum_value_uniqueness);
  validator.add_enum_value_visitor(&validate_enum_value_explicit);

  validator.add_definition_visitor(
      &validate_structured_annotation_type_uniqueness);
  validator.add_definition_visitor(&validate_annotation_scopes);
  return validator;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
