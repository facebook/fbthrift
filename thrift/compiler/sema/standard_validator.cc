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

#include <thrift/compiler/sema/standard_validator.h>

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
#include <thrift/compiler/sema/const_checker.h>
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

bool has_lazy_field(const t_structured& node) {
  for (const auto& field : node.fields()) {
    if (cpp2::is_lazy(&field)) {
      return true;
    }
  }
  return false;
}

// Reports an existing name was redefined within the given parent node.
void report_redef_failure(
    diagnostic_context& ctx,
    const char* kind,
    const std::string& name,
    const std::string& path,
    const t_named& parent,
    const t_node& child,
    const t_node& /*existing*/) {
  // TODO(afuller): Use `existing` to provide more detail in the
  // diagnostic.
  ctx.failure(child, path, [&](auto& o) {
    o << kind << " `" << name << "` is already defined for `" << parent.name()
      << "`.";
  });
}

void report_redef_failure(
    diagnostic_context& ctx,
    const char* kind,
    const std::string& name,
    const t_named& parent,
    const t_node& child,
    const t_node& existing) {
  report_redef_failure(
      ctx, kind, name, ctx.program()->path(), parent, child, existing);
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
  }
}

void validate_boxed_field_attributes(
    diagnostic_context& ctx, const t_field& node) {
  if (gen::cpp::find_ref_type(node) != gen::cpp::reference_type::boxed) {
    return;
  }

  if (!dynamic_cast<const t_union*>(ctx.parent()) &&
      node.qualifier() != t_field_qualifier::optional) {
    ctx.failure([&](auto& o) {
      o << "The `cpp.box` annotation can only be used with optional fields. Make sure `"
        << node.name() << "` is optional.";
    });
  }

  if (node.has_annotation({
          "cpp.ref",
          "cpp2.ref",
          "cpp.ref_type",
          "cpp2.ref_type",
      })) {
    ctx.failure([&](auto& o) {
      o << "The `cpp.box` annotation cannot be combined with the `cpp.ref` or `cpp.ref_type` annotations. Remove one of the annotations from `"
        << node.name() << "`.";
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
    ctx.failure([&](auto& o) {
      o << "Mixin field `" << node.name()
        << "` type must be a struct or union. Found `" << ttype->get_name()
        << "`.";
    });
  }

  if (const auto* parent = dynamic_cast<const t_union*>(ctx.parent())) {
    ctx.failure([&](auto& o) {
      o << "Union `" << parent->name() << "` cannot contain mixin field `"
        << node.name() << "`.";
    });
  } else if (node.qualifier() == t_field_qualifier::optional) {
    // Nothing technically stops us from marking optional field mixin.
    // However, this will bring surprising behavior. e.g. `foo.bar_ref()`
    // might throw `bad_field_access` if `bar` is inside optional mixin
    // field.
    ctx.failure([&](auto& o) {
      o << "Mixin field `" << node.name() << "` cannot be optional.";
    });
  }
}

/*
 * Validates whether all fields which have annotations cpp.ref or cpp2.ref are
 * also optional.
 */
void validate_ref_field_attributes(
    diagnostic_context& ctx, const t_field& node) {
  if (!cpp2::has_ref_annotation(node)) {
    return;
  }

  if (node.qualifier() != t_field_qualifier::optional &&
      dynamic_cast<const t_union*>(ctx.parent()) == nullptr) {
    ctx.warning([&](auto& o) {
      o << "`cpp.ref` field `" << node.name()
        << "` must be optional if it is recursive.";
    });
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

void validate_enum_value(diagnostic_context& ctx, const t_enum_value& node) {
  if (!node.has_value()) {
    ctx.failure([&](auto& o) {
      o << "The enum value, `" << node.name()
        << "`, must have an explicitly assigned value.";
    });
  } else if (node.get_value() < 0 && !ctx.params().allow_neg_enum_vals) {
    ctx.warning([&](auto& o) {
      o << "Negative value supplied for enum value `" << node.name() << "`.";
    });
  }
}

void validate_const_type_and_value(diagnostic_context& ctx, const t_const& c) {
  check_const_rec(ctx, c, &c.type().deref(), c.value());
}

void validate_field_default_value(
    diagnostic_context& ctx, const t_field& field) {
  if (field.get_default_value() != nullptr) {
    check_const_rec(ctx, field, &field.type().deref(), field.default_value());
  }
}

void validate_structured_annotation(
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
    validate_const_type_and_value(ctx, *annot);
  }
}

void validate_uri_uniqueness(diagnostic_context& ctx, const t_program& prog) {
  // TODO: use string_view as map key
  std::unordered_map<std::string, const t_named*> uri_to_node;
  basic_ast_visitor<true, const std::string&> visit;
  visit.add_definition_visitor(
      [&](const std::string& path, const t_named& node) {
        const auto& uri = node.uri();
        if (uri.empty()) {
          return;
        }
        auto result = uri_to_node.emplace(uri, &node);
        if (!result.second) {
          report_redef_failure(
              ctx, "thrift.uri", uri, path, node, node, *result.first->second);
        }
      });
  for (const auto* p : prog.get_included_programs()) {
    visit(p->path(), *p);
  }
  visit(prog.path(), prog);
}

void validate_field_id(diagnostic_context& ctx, const t_field& node) {
  if (!node.has_explicit_id()) {
    ctx.warning([&](auto& o) {
      o << "No field id specified for `" << node.name()
        << "`, resulting protocol may"
        << " have conflicts or not be backwards compatible!";
    });
  }

  if (node.id() == 0 &&
      !node.has_annotation("cpp.deprecated_allow_zero_as_field_id")) {
    ctx.failure([&](auto& o) {
      o << "Zero value (0) not allowed as a field id for `" << node.get_name()
        << "`";
    });
  }

  if (node.id() < t_field::min_id) {
    ctx.failure([&](auto& o) {
      o << "Reserved field id (" << node.id() << ") cannot be used for `"
        << node.name() << "`.";
    });
  }
}

void validate_compatibility_with_lazy_field(
    diagnostic_context& ctx, const t_structured& node) {
  if (!has_lazy_field(node)) {
    return;
  }

  if (node.has_annotation("cpp.methods")) {
    ctx.failure([&](auto& o) {
      o << "cpp.methods is incompatible with lazy deserialization in struct `"
        << node.get_name() << "`";
    });
  }
}

void validate_ref_annotation(diagnostic_context& ctx, const t_field& node) {
  if (node.find_structured_annotation_or_null(
          "facebook.com/thrift/annotation/cpp/Ref") &&
      node.has_annotation(
          {"cpp.ref", "cpp2.ref", "cpp.ref_type", "cpp2.ref_type"})) {
    ctx.failure([&](auto& o) {
      o << "The @cpp.Ref annotation cannot be combined with the `cpp.ref` or `cpp.ref_type` annotations. Remove one of the annotations from `"
        << node.name() << "`.";
    });
  }
}

void validate_adapter_annotation(diagnostic_context& ctx, const t_field& node) {
  const t_const* adapter_annotation = nullptr;
  for (const t_const* annotation : node.structured_annotations()) {
    if (annotation->type()->uri() ==
        "facebook.com/thrift/annotation/cpp/Adapter") {
      adapter_annotation = annotation;
      break;
    }
  }

  if (adapter_annotation &&
      t_typedef::get_first_annotation_or_null(&*node.type(), {"cpp.adapter"})) {
    ctx.failure([&](auto& o) {
      o << "`@cpp.Adapter` cannot be combined with `cpp_adapter` in `"
        << node.name() << "`.";
    });
  }
}

void validate_hack_adapter_annotation(
    diagnostic_context& ctx, const t_field& node) {
  const t_const* field_adapter_annotation = nullptr;
  for (const t_const* annotation : node.structured_annotations()) {
    if (annotation->type()->uri() ==
        "facebook.com/thrift/annotation/hack/ExperimentalAdapter") {
      field_adapter_annotation = annotation;
      break;
    }
  }

  if (field_adapter_annotation &&
      t_typedef::get_first_annotation_or_null(
          &*node.type(), {"hack.adapter"})) {
    ctx.failure([&](auto& o) {
      o << "`@hack.ExperimentalAdapter` cannot be combined with "
           "`hack_adapter` in `"
        << node.name() << "`.";
    });
  }
}

void validate_box_annotation(diagnostic_context& ctx, const t_field& node) {
  if (node.has_annotation("cpp.box")) {
    ctx.warning([&](auto& o) {
      o << "Cpp.box is deprecated. Please use thrift.box annotation instead in `"
        << node.name() << "`.";
    });
  }
}
void validate_box_annotation_in_struct(
    diagnostic_context& ctx, const t_struct& node) {
  if (node.is_struct()) {
    for (const t_field& f : node.fields()) {
      validate_box_annotation(ctx, f);
    }
  }
}

void validate_ref_unique_and_box_annotation(
    diagnostic_context& ctx, const t_field& node) {
  if (cpp2::is_unique_ref(&node)) {
    if (node.has_annotation({"cpp.ref", "cpp2.ref"})) {
      ctx.warning([&](auto& o) {
        o << "cpp.ref, cpp2.ref "
          << "are deprecated. Please use thrift.box annotation instead in `"
          << node.name() << "`.";
      });
    }
    if (node.find_annotation_or_null({"cpp.ref_type", "cpp2.ref_type"})) {
      ctx.warning([&](auto& o) {
        o << "cpp.ref_type = `unique`, cpp2.ref_type = `unique` "
          << "are deprecated. Please use thrift.box annotation instead in `"
          << node.name() << "`.";
      });
    }
    if (node.find_structured_annotation_or_null(
            "facebook.com/thrift/annotation/cpp/Ref")) {
      ctx.warning([&](auto& o) {
        o << "@cpp.Ref{type = cpp.RefType.Unique} "
          << "is deprecated. Please use thrift.box annotation instead in `"
          << node.name() << "`.";
      });
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
  validator.add_structured_definition_visitor(
      &validate_compatibility_with_lazy_field);
  validator.add_structured_definition_visitor(
      &validate_box_annotation_in_struct);
  validator.add_union_visitor(&validate_union_field_attributes);
  validator.add_field_visitor(&validate_field_id);
  validator.add_field_visitor(&validate_mixin_field_attributes);
  validator.add_field_visitor(&validate_boxed_field_attributes);
  validator.add_field_visitor(&validate_ref_field_attributes);
  validator.add_field_visitor(&validate_field_default_value);
  validator.add_field_visitor(&validate_ref_annotation);
  validator.add_field_visitor(&validate_adapter_annotation);
  validator.add_field_visitor(&validate_hack_adapter_annotation);
  validator.add_field_visitor(&validate_ref_unique_and_box_annotation);

  validator.add_enum_visitor(&validate_enum_value_name_uniqueness);
  validator.add_enum_visitor(&validate_enum_value_uniqueness);
  validator.add_enum_value_visitor(&validate_enum_value);

  validator.add_definition_visitor(&validate_structured_annotation);
  validator.add_definition_visitor(&validate_annotation_scopes);

  validator.add_const_visitor(&validate_const_type_and_value);
  validator.add_program_visitor(&validate_uri_uniqueness);
  return validator;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
