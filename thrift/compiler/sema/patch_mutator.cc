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

#include <thrift/compiler/sema/patch_mutator.h>

#include <thrift/compiler/ast/diagnostic_context.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/sema/standard_mutator_stage.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace {

constexpr auto kGeneratePatchUri = "facebook.com/thrift/op/GeneratePatch";
constexpr auto kGenerateOptionalPatchUri =
    "facebook.com/thrift/op/GenerateOptionalPatch";

// TODO(afuller): Index all types by uri, and find them that way.
const char* getPatchTypeName(t_base_type::type base_type) {
  switch (base_type) {
    case t_base_type::type::t_bool:
      return "patch.BoolPatch";
    case t_base_type::type::t_byte:
      return "patch.BytePatch";
    case t_base_type::type::t_i16:
      return "patch.I16Patch";
    case t_base_type::type::t_i32:
      return "patch.I32Patch";
    case t_base_type::type::t_i64:
      return "patch.I64Patch";
    case t_base_type::type::t_float:
      return "patch.FloatPatch";
    case t_base_type::type::t_double:
      return "patch.DoublePatch";
    case t_base_type::type::t_string:
      return "patch.StringPatch";
    case t_base_type::type::t_binary:
      return "patch.BinaryPatch";
    default:
      return "";
  }
}
// TODO(afuller): Index all types by uri, and find them that way.
const char* getOptionalPatchTypeName(t_base_type::type base_type) {
  switch (base_type) {
    case t_base_type::type::t_bool:
      return "patch.OptionalBoolPatch";
    case t_base_type::type::t_byte:
      return "patch.OptionalBytePatch";
    case t_base_type::type::t_i16:
      return "patch.OptionalI16Patch";
    case t_base_type::type::t_i32:
      return "patch.OptionalI32Patch";
    case t_base_type::type::t_i64:
      return "patch.OptionalI64Patch";
    case t_base_type::type::t_float:
      return "patch.OptionalFloatPatch";
    case t_base_type::type::t_double:
      return "patch.OptionalDoublePatch";
    case t_base_type::type::t_string:
      return "patch.OptionalStringPatch";
    case t_base_type::type::t_binary:
      return "patch.OptionalBinaryPatch";
    default:
      return "";
  }
}

// Helper for generating a struct.
struct StructGen {
  // The annotation we are generating for.
  const t_node& annot;
  // The struct to add fields to.
  t_struct& generated;

  // Add a new field to generated, and return it.
  t_field& field(t_field_id id, t_type_ref type, std::string name) {
    generated.append_field(
        std::make_unique<t_field>(type, std::move(name), id));
    t_field& result = generated.fields().back();
    result.set_lineno(annot.lineno());
    return result;
  }

  // A fluent function to box a given field.
  static t_field& box(t_field& node) {
    node.set_qualifier(t_field_qualifier::optional);
    // Box the field, if the underlying type is a struct.
    if (dynamic_cast<const t_struct*>(node.type()->get_true_type())) {
      node.set_annotation("thrift.box");
    }
    return node;
  }
};

// Helper for generating patch structs.
struct PatchGen : StructGen {
  // Standardized patch field ids.
  enum t_patch_field_id : t_field_id {
    kAssignId = 1, // Value Patch
    kEnsureId = 1, // Optional Patch
    kClearId = 2,
    kPatchId = 3,
    kPatchAfterId = 4,
  };

  // 1: optional {type} assign (thrift.box);
  t_field& assign(t_type_ref type) {
    return box(field(kAssignId, type, "assign"));
  }

  // 1: optional {type} ensure (thrift.box);
  t_field& ensure(t_type_ref type) {
    return box(field(kEnsureId, type, "ensure"));
  }

  // 2: bool clear;
  t_field& clear() { return field(kClearId, t_base_type::t_bool(), "clear"); }

  // 3: {patch_type} patch;
  t_field& patch(t_type_ref patch_type) {
    return field(kPatchId, patch_type, "patch");
  }

  // 4: {patch_type} patchAfter;
  t_field& patchAfter(t_type_ref patch_type) {
    return field(kPatchAfterId, patch_type, "patchAfter");
  }
};

t_type_ref resolve_value_type(t_struct& patch_type) {
  // All patch types must have an 'optional Value assign' field.
  t_type_ref result;
  if (const t_field* field = patch_type.get_field_by_name("assign")) {
    // The field type is the value_type.
    result = field->type();
  }
  return result;
}

// Generates an optional patch representation for any patch with the
// @patch.GenerateOptionalPatch annotation.
void generate_optional_patch(
    diagnostic_context& ctx, mutator_context& mctx, t_struct& node) {
  if (auto* annot =
          node.find_structured_annotation_or_null(kGenerateOptionalPatchUri)) {
    // Add a 'optional patch' for the given patch type..
    if (t_type_ref value_type = resolve_value_type(node)) {
      patch_generator::get_for(ctx, mctx).add_optional_patch(
          *annot, value_type, node);
    } else {
      ctx.failure(
          "Could not resolve the 'value' type, needed to generate the optional patch struct.");
    }
  }
}

// Generates a patch representation for any struct with the @patch.GeneratePatch
// annotation.
void generate_struct_patch(
    diagnostic_context& ctx, mutator_context& mctx, t_struct& node) {
  if (auto* annot =
          node.find_structured_annotation_or_null(kGeneratePatchUri)) {
    // Add a 'structure patch' and 'struct value patch' using it.
    auto generator = patch_generator::get_for(ctx, mctx);
    auto& struct_patch = generator.add_structure_patch(*annot, node);
    auto& patch = generator.add_struct_value_patch(*annot, node, struct_patch);

    // Add an 'optional patch' based on the added patch type.
    generator.add_optional_patch(*annot, node, patch);
  }
}

} // namespace

void add_patch_mutators(ast_mutators& mutators) {
  auto& mutator = mutators[standard_mutator_stage::plugin];
  mutator.add_struct_visitor(&generate_struct_patch);
  mutator.add_struct_visitor(&generate_optional_patch);
}

patch_generator& patch_generator::get_for(
    diagnostic_context& ctx, mutator_context& mctx) {
  t_program& program = dynamic_cast<t_program&>(*mctx.root());
  return ctx.cache().get(program, [&]() {
    return std::make_unique<patch_generator>(ctx, program);
  });
}

t_struct& patch_generator::add_optional_patch(
    const t_node& annot, t_type_ref value_type, t_struct& patch_type) {
  PatchGen gen{{annot, gen_prefix_struct(annot, patch_type, "Optional")}};
  gen.generated.set_annotation(
      "cpp.adapter", "::apache::thrift::op::detail::OptionalPatchAdapter");
  gen.clear().set_doc(
      "If the optional value should be cleared. Applied first.");
  gen.patch(patch_type)
      .set_doc("The patch to apply to any set value. Applied second.");
  gen.ensure(value_type)
      .set_doc(
          "The value with which to initialize any unset value. Applied third.");
  gen.patchAfter(patch_type)
      .set_doc(
          "The patch to apply to any set value, including newly set values. Applied fourth.");
  return gen.generated;
}

t_struct& patch_generator::add_structure_patch(
    const t_const& annot, t_structured& orig) {
  StructGen gen{annot, gen_suffix_struct(annot, orig, "Patch")};
  for (const auto& field : orig.fields()) {
    if (t_type_ref patch_type = find_patch_type(annot, field)) {
      gen.field(field.id(), patch_type, field.name());
    } else {
      ctx_.warning(field, "Could not resolve patch type for field.");
    }
  }
  return gen.generated;
}

t_struct& patch_generator::add_struct_value_patch(
    const t_node& annot, t_struct& value_type, t_type_ref patch_type) {
  PatchGen gen{{annot, gen_suffix_struct(annot, value_type, "ValuePatch")}};
  gen.assign(value_type)
      .set_doc(
          "Assigns to a given struct. If set, all other operations are ignored.\n");
  gen.clear().set_doc("Clears a given struct. Applied first.\n");
  gen.patch(patch_type).set_doc("Patches a given struct. Applied second.\n");
  gen.generated.set_annotation(
      "cpp.adapter", "::apache::thrift::op::detail::StructPatchAdapter");
  return gen.generated;
}

t_type_ref patch_generator::find_patch_type(
    const t_const& annot, const t_field& field) const {
  // Base types use a shared representation defined in patch.thrift.
  const auto* type = field.type()->get_true_type();
  if (auto* base_type = dynamic_cast<const t_base_type*>(type)) {
    const char* name = field.qualifier() == t_field_qualifier::optional
        ? getOptionalPatchTypeName(base_type->base_type())
        : getPatchTypeName(base_type->base_type());
    if (const auto* result = program_.scope()->find_type(name)) {
      return t_type_ref::from_ptr(result);
    }
    // TODO(afuller): This look up hack only works for 'built-in' patch types.
    // Use a shared uri type registry instead.
    if (const auto* result =
            annot.type()->program()->scope()->find_type(name)) {
      return t_type_ref::from_ptr(result);
    }

    // These type should always be availabile because the are defined along
    // side the annoation used to trigger patch generation.
    ctx_.failure(field, [&](auto& os) {
      os << "Could not find expected patch type: " << name;
    });
  } else if (auto* structured = dynamic_cast<const t_structured*>(type)) {
    // Try to find the generated patch type.
    std::string name = structured->name() + "ValuePatch";
    if (field.qualifier() == t_field_qualifier::optional) {
      name = "Optional" + std::move(name);
    }
    // It should be in the same program as the type itself.
    name = structured->program()->name() + "." + std::move(name);
    if (auto patch_type =
            t_type_ref::from_ptr(program_.scope()->find_type(name))) {
      return patch_type;
    }
    ctx_.warning(
        field, "Could not find expected patch type: " + std::move(name));
  }

  // Could not resolve the patch type.
  return {};
}

t_struct& patch_generator::gen_struct(
    const t_node& annot, std::string name, std::string uri) {
  auto generated = std::make_unique<t_struct>(&program_, std::move(name));
  t_struct* ptr = generated.get();
  generated->set_uri(std::move(uri));
  // Attribute the new struct to the anntation.
  generated->set_lineno(annot.lineno());
  program_.scope()->add_type(
      program_.name() + "." + generated->name(), generated.get());
  program_.add_definition(std::move(generated));
  return *ptr;
}

t_struct& patch_generator::gen_suffix_struct(
    const t_node& annot, const t_named& orig, const char* suffix) {
  ctx_.failure_if(
      orig.uri().empty(), annot, "URI required to support patching.");
  t_struct& generated =
      gen_struct(annot, orig.name() + suffix, orig.uri() + suffix);
  if (const auto* cpp_name = orig.find_annotation_or_null("cpp.name")) {
    generated.set_annotation("cpp.name", *cpp_name + suffix);
  }
  return generated;
}

t_struct& patch_generator::gen_prefix_struct(
    const t_node& annot, const t_named& orig, const char* prefix) {
  t_struct& generated = gen_struct(
      annot, prefix + orig.name(), prefix_uri_name(orig.uri(), prefix));
  if (const auto* cpp_name = orig.find_annotation_or_null("cpp.name")) {
    generated.set_annotation("cpp.name", prefix + *cpp_name);
  }
  return generated;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
