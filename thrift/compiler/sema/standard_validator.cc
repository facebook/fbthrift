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

#include <thrift/compiler/sema/standard_validator.h>

#include <algorithm>
#include <string>
#include <unordered_map>

#include <fmt/ranges.h>

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
#include <thrift/compiler/ast/t_typedef.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/gen/cpp/reference_type.h>
#include <thrift/compiler/lib/cpp2/util.h>
#include <thrift/compiler/sema/const_checker.h>
#include <thrift/compiler/sema/scope_validator.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

constexpr auto kCppRefUri = "facebook.com/thrift/annotation/cpp/Ref";
constexpr auto kCppAdapterUri = "facebook.com/thrift/annotation/cpp/Adapter";
constexpr auto kHackAdapterUri = "facebook.com/thrift/annotation/hack/Adapter";
constexpr auto kReserveIdsUri = "facebook.com/thrift/annotation/ReserveIds";
constexpr auto kCppUnstructuredAdapter = "cpp.adapter";
constexpr auto kHackUnstructuredAdapter = "hack.adapter";

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
      ctx, kind, name, ctx.program().path(), parent, child, existing);
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

// Helper for validating the adapters
class adapter_checker {
 public:
  explicit adapter_checker(diagnostic_context& ctx) : ctx_(ctx) {}

  // Checks if adapter name is provided
  // Do not allow composing two structured annotations on typedef
  void check(
      const t_named& node,
      const char* structured_adapter_annotation,
      const char* structured_adapter_annotation_error_name) {
    const t_const* adapter_annotation =
        node.find_structured_annotation_or_null(structured_adapter_annotation);
    if (!adapter_annotation) {
      return;
    }

    try {
      adapter_annotation->get_value_from_structured_annotation("name");
    } catch (const std::exception& e) {
      ctx_.failure([&](auto& o) { o << e.what(); });
      return;
    }

    // TODO (dokwon): Do not allow composing unstructured adapter annotation on
    // typedef as well.
    if (const auto* typedf = dynamic_cast<const t_typedef*>(&node)) {
      if (t_typedef::get_first_structured_annotation_or_null(
              &*typedf->type(), structured_adapter_annotation)) {
        ctx_.failure([&](auto& o) {
          o << "The `" << structured_adapter_annotation_error_name
            << "` annotation cannot be annotated more than once in all typedef levels in `"
            << node.name() << "`.";
        });
      }
    }
  }

  void check(
      const t_named&& node,
      const char* structured_adapter_annotation,
      const char* structured_adapter_annotation_error_name) = delete;

  // Do not allow composing structured annotation on field/typedef
  // and unstructured annotation on typedef/type
  void check(
      const t_field& field,
      const char* structured_adapter_annotation,
      const char* unstructured_adapter_annotation,
      const char* structured_adapter_annotation_error_name,
      bool disallow_structured_annotations_on_both_field_and_typedef) {
    if (!field.type().resolved()) {
      return;
    }
    auto type = field.type().get_type();

    bool structured_annotation_on_field =
        field.find_structured_annotation_or_null(structured_adapter_annotation);

    bool structured_annotation_on_typedef =
        t_typedef::get_first_structured_annotation_or_null(
            type, structured_adapter_annotation) != nullptr;

    if (disallow_structured_annotations_on_both_field_and_typedef &&
        structured_annotation_on_field && structured_annotation_on_typedef) {
      ctx_.failure([&](auto& o) {
        o << "`" << structured_adapter_annotation_error_name
          << "` cannot be applied on both field and typedef in `"
          << field.name() << "`.";
      });
    }

    if (structured_annotation_on_typedef || structured_annotation_on_field) {
      if (t_typedef::get_first_annotation_or_null(
              type, {unstructured_adapter_annotation})) {
        ctx_.failure([&](auto& o) {
          o << "`" << structured_adapter_annotation_error_name
            << "` cannot be combined with `" << unstructured_adapter_annotation
            << "` in `" << field.name() << "`.";
        });
      }
    }
  }

  void check(
      const t_field&& field,
      const char* structured_adapter_annotation,
      const char* unstructured_adapter_annotation,
      const char* structured_adapter_annotation_error_name,
      bool disallow_structured_annotations_on_both_field_and_typedef) = delete;

 private:
  diagnostic_context& ctx_;
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
    ctx.check(
        dynamic_cast<const t_exception*>(except_type),
        except,
        "Non-exception type, `{}`, in throws.",
        except_type->name());
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
    if (field.qualifier() != t_field_qualifier::none) {
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

  ctx.check(
      dynamic_cast<const t_union*>(ctx.parent()) ||
          node.qualifier() == t_field_qualifier::optional,
      "The `cpp.box` annotation can only be used with optional fields. Make sure `{}` is optional.",
      node.name());

  ctx.check(
      !node.has_annotation({
          "cpp.ref",
          "cpp2.ref",
          "cpp.ref_type",
          "cpp2.ref_type",
      }),
      "The `cpp.box` annotation cannot be combined with the `cpp.ref` or `cpp.ref_type` annotations. Remove one of the annotations from `{}`.",
      node.name());
}

// Checks the attributes of a mixin field.
void validate_mixin_field_attributes(
    diagnostic_context& ctx, const t_field& node) {
  if (!cpp2::is_mixin(node)) {
    return;
  }

  auto* ttype = node.type()->get_true_type();
  ctx.check(
      typeid(*ttype) == typeid(t_struct) || typeid(*ttype) == typeid(t_union),
      "Mixin field `{}` type must be a struct or union. Found `{}`.",
      node.name(),
      ttype->get_name());

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
    ctx.warning(
        "`cpp.ref` field `{}` must be optional if it is recursive.",
        node.name());
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
    ctx.check(
        prev.second,
        value,
        "Duplicate value `{}={}` with value `{}` in enum `{}`.",
        value.name(),
        value.get_value(),
        prev.first->second->name(),
        node.name());
  }
}

void validate_enum_value(diagnostic_context& ctx, const t_enum_value& node) {
  if (!node.has_value()) {
    ctx.failure([&](auto& o) {
      o << "The enum value, `" << node.name()
        << "`, must have an explicitly assigned value.";
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
        if (uri.empty() || uri == t_named::kTransitiveUri) {
          return;
        }
        auto result = uri_to_node.emplace(uri, &node);
        if (!result.second) {
          report_redef_failure(
              ctx, "Thrift URI", uri, path, node, node, *result.first->second);
        }
      });
  for (const auto* p : prog.get_included_programs()) {
    visit(p->path(), *p);
  }
  visit(prog.path(), prog);
}

void validate_field_id(diagnostic_context& ctx, const t_field& node) {
  if (node.explicit_id() != node.id()) {
    ctx.warning(
        "No field id specified for `{}`, resulting protocol may have conflicts "
        "or not be backwards compatible!",
        node.name());
  }

  ctx.check(
      node.id() != 0 ||
          node.has_annotation("cpp.deprecated_allow_zero_as_field_id"),
      "Zero value (0) not allowed as a field id for `{}`",
      node.get_name());

  ctx.check(
      node.id() >= t_field::min_id || node.is_injected(),
      "Reserved field id ({}) cannot be used for `{}`.",
      node.id(),
      node.name());
}

void validate_compatibility_with_lazy_field(
    diagnostic_context& ctx, const t_structured& node) {
  if (has_lazy_field(node) && node.has_annotation("cpp.methods")) {
    ctx.failure([&](auto& o) {
      o << "cpp.methods is incompatible with lazy deserialization in struct `"
        << node.get_name() << "`";
    });
  }
}

void validate_ref_annotation(diagnostic_context& ctx, const t_field& node) {
  if (node.find_structured_annotation_or_null(kCppRefUri) &&
      node.has_annotation(
          {"cpp.ref", "cpp2.ref", "cpp.ref_type", "cpp2.ref_type"})) {
    ctx.failure([&](auto& o) {
      o << "The @cpp.Ref annotation cannot be combined with the `cpp.ref` or `cpp.ref_type` annotations. Remove one of the annotations from `"
        << node.name() << "`.";
    });
  }
}

void validate_cpp_adapter_annotation(
    diagnostic_context& ctx, const t_named& node) {
  adapter_checker(ctx).check(node, kCppAdapterUri, "@cpp.Adapter");
}

void validate_hack_adapter_annotation(
    diagnostic_context& ctx, const t_named& node) {
  adapter_checker(ctx).check(node, kHackAdapterUri, "@hack.Adapter");
}

void validate_box_annotation(
    diagnostic_context& ctx, const t_structured& node) {
  if (node.generated()) {
    return;
  }

  for (const auto& field : node.fields()) {
    if (field.has_annotation({"cpp.box", "thrift.box"})) {
      ctx.warning(
          field,
          "cpp.box and thrift.box are deprecated. Please use @thrift.Box "
          "annotation instead in `{}`.",
          field.name());
    }
  }
}

void validate_ref_unique_and_box_annotation(
    diagnostic_context& ctx, const t_field& node) {
  const t_const* adapter_annotation =
      node.find_structured_annotation_or_null(kCppAdapterUri);

  if (cpp2::is_unique_ref(&node)) {
    if (node.has_annotation({"cpp.ref", "cpp2.ref"})) {
      if (adapter_annotation) {
        ctx.failure([&](auto& o) {
          o << "cpp.ref, cpp2.ref "
            << "are deprecated. Please use @thrift.Box annotation instead in `"
            << node.name() << "` with @cpp.Adapter.";
        });
      } else {
        ctx.warning(
            "cpp.ref, cpp2.ref are deprecated. Please use @thrift.Box "
            "annotation instead in `{}`.",
            node.name());
      }
    }
    if (node.has_annotation({"cpp.ref_type", "cpp2.ref_type"})) {
      if (adapter_annotation) {
        ctx.failure([&](auto& o) {
          o << "cpp.ref_type = `unique`, cpp2.ref_type = `unique` "
            << "are deprecated. Please use @thrift.Box annotation instead in `"
            << node.name() << "` with @cpp.Adapter.";
        });
      } else {
        ctx.warning(
            "cpp.ref_type = `unique`, cpp2.ref_type = `unique` "
            "are deprecated. Please use @thrift.Box annotation instead in "
            "`{}`.",
            node.name());
      }
    }
    if (node.find_structured_annotation_or_null(kCppRefUri) != nullptr) {
      if (adapter_annotation) {
        ctx.failure([&](auto& o) {
          o << "@cpp.Ref{type = cpp.RefType.Unique} "
            << "is deprecated. Please use @thrift.Box annotation instead in `"
            << node.name() << "` with @cpp.Adapter.";
        });
      } else {
        ctx.warning(
            "@cpp.Ref{{type = cpp.RefType.Unique}} is deprecated. Please use "
            "@thrift.Box annotation instead in `{}`.",
            node.name());
      }
    }
  }
}

void validate_function_priority_annotation(
    diagnostic_context& ctx, const t_node& node) {
  if (auto* priority = node.find_annotation_or_null("priority")) {
    const std::string choices[] = {
        "HIGH_IMPORTANT", "HIGH", "IMPORTANT", "NORMAL", "BEST_EFFORT"};
    auto* end = choices + sizeof(choices) / sizeof(choices[0]);
    ctx.check(
        std::find(choices, end, *priority) != end,
        "Bad priority '{}'. Choose one of {}.",
        *priority,
        choices);
  }
}

void validate_exception_php_annotations(
    diagnostic_context& ctx, const t_exception& node) {
  constexpr const char* annotations[] = {"message", "code"};
  for (const auto& annotation : annotations) {
    if (node.get_field_by_name(annotation) != nullptr &&
        strcmp(annotation, node.get_annotation(annotation).c_str()) != 0) {
      ctx.warning(
          "Some generators (e.g. PHP) will ignore annotation '{}' as it is "
          "also used as field",
          annotation);
    }
  }

  // Check that value of "message" annotation is
  // - a valid member of struct
  // - of type STRING
  if (node.has_annotation("message")) {
    const std::string& v = node.get_annotation("message");
    const auto* field = node.get_field_by_name(v);
    if (field == nullptr) {
      ctx.failure([&](auto& o) {
        o << "member specified as exception 'message' should be a valid"
          << " struct member, '" << v << "' in '" << node.name() << "' is not";
      });
    } else if (!field->get_type()->is_string_or_binary()) {
      ctx.failure([&](auto& o) {
        o << "member specified as exception 'message' should be of type "
          << "STRING, '" << v << "' in '" << node.name() << "' is not";
      });
    }
  }
}

void validate_oneway_function(diagnostic_context& ctx, const t_function& node) {
  if (!node.is_oneway()) {
    return;
  }

  ctx.check(
      node.return_type().get_type() != nullptr &&
          node.return_type().get_type()->is_void() &&
          !node.returned_interaction(),
      "Oneway methods must have void return type: {}",
      node.name());

  ctx.check(
      t_throws::is_null_or_empty(node.exceptions()),
      "Oneway methods can't throw exceptions: {}",
      node.name());
}

void validate_stream_exceptions_return_type(
    diagnostic_context& ctx, const t_function& node) {
  if (t_throws::is_null_or_empty(node.get_stream_xceptions())) {
    return;
  }

  ctx.check(
      dynamic_cast<const t_stream_response*>(node.return_type().get_type()),
      "`stream throws` only valid on stream methods: {}",
      node.name());
}

void validate_interaction_factories(
    diagnostic_context& ctx, const t_function& node) {
  if (node.is_interaction_member() && node.returned_interaction()) {
    ctx.failure([&](auto& o) {
      o << "Nested interactions are forbidden: " << node.name();
    });
  }
}

void validate_cpp_field_adapter_annotation(
    diagnostic_context& ctx, const t_field& field) {
  adapter_checker(ctx).check(
      field,
      kCppAdapterUri,
      kCppUnstructuredAdapter,
      "@cpp.Adapter",
      false /* disallow_structured_annotations_on_both_field_and_typedef */);
}

void validate_hack_field_adapter_annotation(
    diagnostic_context& ctx, const t_field& field) {
  adapter_checker(ctx).check(
      field,
      kHackAdapterUri,
      kHackUnstructuredAdapter,
      "@hack.Adapter",
      true /* disallow_structured_annotations_on_both_field_and_typedef */);
}

class reserved_ids_checker {
 public:
  explicit reserved_ids_checker(diagnostic_context& ctx) : ctx_(ctx) {}

  void check(const t_structured& node) {
    auto reserved_ids = get_reserved_ids(node);
    for (const auto& field : node.fields()) {
      ctx_.check(
          reserved_ids.count(field.id()) == 0,
          "Fields in {} cannot use reserved ids: {}",
          node.name(),
          field.id());
    }
  }

  void check(const t_enum& node) {
    auto reserved_ids = get_reserved_ids(node);
    for (const auto& enum_value : node.values()) {
      ctx_.check(
          reserved_ids.count(enum_value.get_value()) == 0,
          "Enum values in {} cannot use reserved ids: {}",
          node.name(),
          enum_value.get_value());
    }
  }

 private:
  diagnostic_context& ctx_;

  // Gets all the reserved ids annotated on this node. Returns
  // empty set if the annotation is not present.
  std::unordered_set<int32_t> get_reserved_ids(const t_type& node) {
    std::unordered_set<int32_t> reserved_ids;

    auto* annotation = node.find_structured_annotation_or_null(kReserveIdsUri);
    if (annotation == nullptr) {
      return reserved_ids;
    }

    // Take the union of the list of tag values in `ids` and the range of
    // of values from `id_ranges`
    if (auto ids =
            annotation->get_value_from_structured_annotation_or_null("ids");
        ids != nullptr) {
      ctx_.check(
          ids->get_type() == t_const_value::t_const_value_type::CV_LIST,
          "Field ids must be a list of integers, annotated on {}",
          node.name());
      for (const auto* id : ids->get_list()) {
        ctx_.check(
            id->get_type() == t_const_value::t_const_value_type::CV_INTEGER,
            "Field ids must be a list of integers, annotated on {}",
            node.name());
        reserved_ids.insert(id->get_integer());
      }
    }
    if (auto id_ranges =
            annotation->get_value_from_structured_annotation_or_null(
                "id_ranges");
        id_ranges != nullptr) {
      ctx_.check(
          id_ranges->get_type() == t_const_value::t_const_value_type::CV_MAP,
          "Field id_ranges must be a map of integer to integer, annotated on {}",
          node.name());
      for (const auto& [id_range_begin, id_range_end] : id_ranges->get_map()) {
        ctx_.check(
            id_range_begin->get_type() ==
                    t_const_value::t_const_value_type::CV_INTEGER &&
                id_range_begin->get_type() ==
                    t_const_value::t_const_value_type::CV_INTEGER,
            "Field id_ranges must be a map of integer to integer, annotated on {}",
            node.name());
        ctx_.check(
            id_range_begin->get_integer() < id_range_end->get_integer(),
            "For each (start: end) in id_ranges, we must have start < end. Got ({}: {}), annotated on {}",
            id_range_begin->get_integer(),
            id_range_end->get_integer(),
            node.name());
        for (int i = id_range_begin->get_integer();
             i <= id_range_end->get_integer();
             ++i) {
          reserved_ids.insert(i);
        }
      }
    }
    return reserved_ids;
  }
};

void validate_reserved_ids_structured(
    diagnostic_context& ctx, const t_structured& node) {
  reserved_ids_checker(ctx).check(node);
}

void validate_reserved_ids_enum(diagnostic_context& ctx, const t_enum& node) {
  reserved_ids_checker(ctx).check(node);
}
} // namespace

ast_validator standard_validator() {
  ast_validator validator;
  validator.add_interface_visitor(&validate_interface_function_name_uniqueness);
  validator.add_interface_visitor(&validate_function_priority_annotation);
  validator.add_service_visitor(
      &validate_extends_service_function_name_uniqueness);
  validator.add_throws_visitor(&validate_throws_exceptions);
  validator.add_function_visitor(&validate_oneway_function);
  validator.add_function_visitor(&validate_stream_exceptions_return_type);
  validator.add_function_visitor(&validate_function_priority_annotation);
  validator.add_function_visitor(&validate_interaction_factories);

  validator.add_structured_definition_visitor(&validate_field_names_uniqueness);
  validator.add_structured_definition_visitor(
      &validate_compatibility_with_lazy_field);
  validator.add_structured_definition_visitor(&validate_box_annotation);
  validator.add_structured_definition_visitor(
      &validate_reserved_ids_structured);
  validator.add_union_visitor(&validate_union_field_attributes);
  validator.add_exception_visitor(&validate_exception_php_annotations);
  validator.add_field_visitor(&validate_field_id);
  validator.add_field_visitor(&validate_mixin_field_attributes);
  validator.add_field_visitor(&validate_boxed_field_attributes);
  validator.add_field_visitor(&validate_ref_field_attributes);
  validator.add_field_visitor(&validate_field_default_value);
  validator.add_field_visitor(&validate_ref_annotation);
  validator.add_field_visitor(&validate_ref_unique_and_box_annotation);
  validator.add_field_visitor(&validate_cpp_field_adapter_annotation);
  validator.add_field_visitor(&validate_hack_field_adapter_annotation);

  validator.add_enum_visitor(&validate_enum_value_name_uniqueness);
  validator.add_enum_visitor(&validate_enum_value_uniqueness);
  validator.add_enum_visitor(&validate_reserved_ids_enum);
  validator.add_enum_value_visitor(&validate_enum_value);

  validator.add_definition_visitor(&validate_structured_annotation);
  validator.add_definition_visitor(&validate_annotation_scopes);
  validator.add_definition_visitor(&validate_cpp_adapter_annotation);
  validator.add_definition_visitor(&validate_hack_adapter_annotation);

  validator.add_const_visitor(&validate_const_type_and_value);
  validator.add_program_visitor(&validate_uri_uniqueness);
  return validator;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
