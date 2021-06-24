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

#include <thrift/compiler/ast/name_index.h>
#include <thrift/compiler/ast/t_enum.h>
#include <thrift/compiler/ast/t_enum_value.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_function.h>
#include <thrift/compiler/ast/t_interface.h>
#include <thrift/compiler/ast/t_named.h>
#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_structured.h>
#include <thrift/compiler/ast/t_throws.h>
#include <thrift/compiler/ast/t_type.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/gen/cpp/reference_type.h>
#include <thrift/compiler/lib/cpp2/util.h>
#include <thrift/compiler/sema/scope_validator.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

const t_structured* get_mixin_type(const t_field& field) {
  if (cpp2::is_mixin(field)) {
    return dynamic_cast<const t_structured*>(field.type()->get_true_type());
  }
  return nullptr;
}

// Reports an existing name was redefined within the given parent node.
void report_redef_failure(
    diagnostic_context& ctx,
    const char* kind,
    const std::string& name,
    const t_named& parent,
    const t_node& child,
    const t_node& /*existing*/) {
  // TODO(afuller): Use `existing` to provide more detail in the
  // diagnostic.
  ctx.failure(child, [&](auto& o) {
    o << kind << " `" << name << "` is already defined for `" << parent.name()
      << "`.";
  });
}

// Helper for checking for the redefinition of a name in the context of a node.
class redef_checker {
 public:
  redef_checker(
      diagnostic_context& ctx, const char* kind, const t_named& parent)
      : ctx_(ctx), kind_(kind), parent_(parent) {}

  // Checks if the given `name`, derived from `node` via `child`, has already
  // been defined.
  //
  // For example, a mixin field causes all fields of the mixin type to be
  // inherited. In this case 'node' wold be the mixin type, from which `name`
  // was derived, while `child` is the mixin field that caused the name to be
  // inherited.
  void check(
      const std::string& name, const t_named& node, const t_node& child) {
    if (const auto* existing = seen_.put(name, node)) {
      if (&node == &parent_ && existing == &parent_) {
        // The degenerate case where parent_ is conflicting with itself.
        report_redef_failure(ctx_, kind_, name, parent_, child, *existing);
      } else {
        ctx_.failure(child, [&](auto& o) {
          o << kind_ << " `" << node.name() << "." << name << "` and `"
            << existing->name() << "." << name
            << "` can not have same name in `" << parent_.name() << "`.";
        });
      }
    }
  }
  void check(std::string&&, const t_named&, const t_node&) = delete;
  void check(const std::string&, t_named&&, const t_node&) = delete;

  // Helpers for the common case where the names are from child t_nameds of
  // the parent.
  //
  // For example, all functions in an interface.
  void check(const t_named& child) {
    if (const auto* existing = seen_.put(child)) {
      report_redef_failure(
          ctx_, kind_, child.name(), parent_, child, *existing);
    }
  }
  void check(t_named&& child) = delete;

  template <typename Cs>
  void check_all(const Cs& children) {
    for (const t_named& child : children) {
      check(child);
    }
  }

 private:
  diagnostic_context& ctx_;
  const char* kind_;
  const t_named& parent_;

  name_index<t_named> seen_;
};

struct service_metadata {
  name_index<t_service> function_name_to_service;

  service_metadata(node_metadata_cache& cache, const t_service& node) {
    if (node.extends() != nullptr) {
      // Add all the inherited functions.
      function_name_to_service =
          cache.get<service_metadata>(*node.extends()).function_name_to_service;
    }
    // Add all the directly defined functions.
    for (const auto& function : node.functions()) {
      function_name_to_service.put(function.name(), node);
    }
  }
};

struct structured_metadata {
  name_index<t_structured> field_name_to_parent;

  structured_metadata(node_metadata_cache& cache, const t_structured& node) {
    for (const auto& field : node.fields()) {
      if (const auto* mixin = get_mixin_type(field)) {
        // Add all the inherited mixin fields from field.
        auto mixin_metadata = cache.get<structured_metadata>(*mixin);
        field_name_to_parent.put_all(mixin_metadata.field_name_to_parent);
      }
      // Add the directly defined field.
      field_name_to_parent.put(field.name(), node);
    }
  }
};

void validate_interface_function_name_uniqueness(
    diagnostic_context& ctx, const t_interface& node) {
  // Check for a redefinition of a function in the same interface.
  redef_checker(ctx, "Function", node).check_all(node.functions());
}

// Checks for a redefinition of an inherited function.
void validate_extends_service_function_name_uniqueness(
    diagnostic_context& ctx, const t_service& node) {
  if (node.extends() == nullptr) {
    return;
  }

  const auto& extends_metadata =
      ctx.cache().get<service_metadata>(*node.extends());
  for (const auto& function : node.functions()) {
    if (const auto* existing_service =
            extends_metadata.function_name_to_service.find(function.name())) {
      ctx.failure(function, [&](auto& o) {
        o << "Function `" << node.name() << "." << function.name()
          << "` redefines `" << existing_service->get_full_name() << "."
          << function.name() << "`.";
      });
    }
  }
}

void validate_throws_exceptions(diagnostic_context& ctx, const t_throws& node) {
  for (const auto& except : node.fields()) {
    auto except_type = except.type()->get_true_type();
    if (dynamic_cast<const t_exception*>(except_type) == nullptr) {
      ctx.failure(except, [&](auto& o) {
        o << "Non-exception type, `" << except_type->name() << "`, in throws.";
      });
    }
  }
}

// Checks for a redefinition of a field in the same t_structured, including
// those inherited via mixin fields.
void validate_field_names_uniqueness(
    diagnostic_context& ctx, const t_structured& node) {
  redef_checker checker(ctx, "Field", node);
  for (const auto& field : node.fields()) {
    // Check the directly defined field.
    checker.check(field.name(), node, field);

    // Check any transtively defined fields via a mixin annotation.
    if (const auto* mixin = get_mixin_type(field)) {
      const auto& mixin_metadata = ctx.cache().get<structured_metadata>(*mixin);
      mixin_metadata.field_name_to_parent.for_each(
          [&](const std::string& name, const t_structured& parent) {
            checker.check(name, parent, field);
          });
    }
  }
}

void validate_struct_except_field_attributes(
    diagnostic_context& ctx, const t_structured& node) {
  for (const auto& field : node.fields()) {
    if (gen::cpp::find_ref_type(field) == gen::cpp::reference_type::boxed &&
        field.qualifier() != t_field_qualifier::optional) {
      ctx.failure(field, [&](auto& o) {
        o << "The `cpp.box` annotation can only be used with optional fields. Make sure `"
          << field.name() << "` is optional.";
      });
    }
  }
}

// Checks the attributes of fields in a union.
void validate_union_field_attributes(
    diagnostic_context& ctx, const t_union& node) {
  for (const auto& field : node.fields()) {
    if (field.qualifier() != t_field_qualifier::unspecified) {
      auto qual = field.qualifier() == t_field_qualifier::required ? "required"
                                                                   : "optional";
      ctx.failure(field, [&](auto& o) {
        o << "Unions cannot contain qualified fields. Remove `" << qual
          << "` qualifier from field `" << field.name() << "`.";
      });
    }
    if (cpp2::is_mixin(field)) {
      ctx.failure(field, [&](auto& o) {
        o << "Union `" << node.name() << "` cannot contain mixin field `"
          << field.name() << "`.";
      });
    }
    if (gen::cpp::find_ref_type(field) == gen::cpp::reference_type::boxed) {
      // TODO(afuller): Support cpp.box on union fields.
      ctx.failure(field, [&](auto& o) {
        o << "Unions cannot contain fields with the `cpp.box` annotation. Remove the annotation from `"
          << field.name() << "`.";
      });
    }
  }
}

void validate_boxed_field_attributes(
    diagnostic_context& ctx, const t_field& field) {
  if (gen::cpp::find_ref_type(field) != gen::cpp::reference_type::boxed) {
    return;
  }

  if (field.has_annotation({
          "cpp.ref",
          "cpp2.ref",
          "cpp.ref_type",
          "cpp2.ref_type",
      })) {
    ctx.failure(field, [&](auto& o) {
      o << "The `cpp.box` annotation cannot be combined with the `cpp.ref` or `cpp.ref_type` annotations. Remove one of the annotations from `"
        << field.name() << "`.";
    });
  }
}

// Checks the attributes of a mixin field.
void validate_mixin_field_attributes(
    diagnostic_context& ctx, const t_field& node) {
  if (!cpp2::is_mixin(node)) {
    return;
  }

  auto* ttype = node.type()->get_true_type();
  if (typeid(*ttype) != typeid(t_struct) && typeid(*ttype) != typeid(t_union)) {
    ctx.failure(node, [&](auto& o) {
      o << "Mixin field `" << node.name()
        << "` type must be a struct or union. Found `" << ttype->get_name()
        << "`.";
    });
  }

  if (node.qualifier() == t_field_qualifier::optional) {
    // Nothing technically stops us from marking optional field mixin.
    // However, this will bring surprising behavior. e.g. `foo.bar_ref()`
    // might throw `bad_field_access` if `bar` is inside optional mixin
    // field.
    ctx.failure(node, [&](auto& o) {
      o << "Mixin field `" << node.name() << "` cannot be optional.";
    });
  }
}

/*
 * Validates whether all fields in a t_struct which have annotations
 * cpp.ref or cpp2.ref are also optional.
 */
void validate_struct_optional_refs(
    diagnostic_context& ctx, const t_struct& node) {
  for (const auto& field : node.fields()) {
    if (cpp2::has_ref_annotation(field) &&
        field.qualifier() != t_field_qualifier::optional) {
      ctx.warning(field, [&](auto& o) {
        o << "`cpp.ref` field `" << field.name()
          << "` must be optional if it is recursive.";
      });
    }
  }
}

void validate_enum_value_name_uniqueness(
    diagnostic_context& ctx, const t_enum& node) {
  redef_checker(ctx, "Enum value", node).check_all(node.values());
}

void validate_enum_value_uniqueness(
    diagnostic_context& ctx, const t_enum& node) {
  std::unordered_map<int32_t, const t_enum_value*> values;
  for (const auto& value : node.values()) {
    auto prev = values.emplace(value.get_value(), &value);
    if (!prev.second) {
      ctx.failure(value, [&](auto& o) {
        o << "Duplicate value `" << value.name() << "=" << value.get_value()
          << "` with value `" << prev.first->second->name() << "` in enum `"
          << node.name() << "`.";
      });
    }
  }
}

void validate_enum_value_explicit(
    diagnostic_context& ctx, const t_enum_value& node) {
  if (!node.has_value()) {
    ctx.failure(node, [&](auto& o) {
      o << "The enum value, `" << node.name()
        << "`, must have an explicitly assigned value.";
    });
  }
}

void validate_structured_annotation_type_uniqueness(
    diagnostic_context& ctx, const t_named& node) {
  std::unordered_map<const t_type*, const t_const*> seen;
  for (const auto* annot : node.structured_annotations()) {
    auto result = seen.emplace(&annot->type().deref(), annot);
    if (!result.second) {
      report_redef_failure(
          ctx,
          "Structured annotation",
          result.first->first->name(),
          node,
          *annot,
          *result.first->second);
    }
  }
}

} // namespace

ast_validator standard_validator() {
  ast_validator validator;
  validator.add_interface_visitor(&validate_interface_function_name_uniqueness);
  validator.add_service_visitor(
      &validate_extends_service_function_name_uniqueness);
  validator.add_throws_visitor(&validate_throws_exceptions);

  validator.add_structured_definition_visitor(&validate_field_names_uniqueness);
  validator.add_union_visitor(&validate_union_field_attributes);
  validator.add_struct_visitor(&validate_struct_except_field_attributes);
  validator.add_exception_visitor(&validate_struct_except_field_attributes);
  validator.add_field_visitor(&validate_mixin_field_attributes);
  validator.add_field_visitor(&validate_boxed_field_attributes);

  validator.add_struct_visitor(&validate_struct_optional_refs);

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
