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

struct StructGen {
  // The annotation we are generating for.
  const t_node& annot;

  // Generate a new field.
  std::unique_ptr<t_field> field(
      t_field_id id, t_type_ref type, std::string name) {
    auto result = std::make_unique<t_field>(type, std::move(name), id);
    result->set_lineno(annot.lineno());
    return result;
  }

  // A fluent function to add a doc string to a node.
  template <typename N>
  static std::unique_ptr<N> withDoc(std::string doc, std::unique_ptr<N> node) {
    node->set_doc(std::move(doc));
    return node;
  }

  // A fluent function to box a given field.
  static std::unique_ptr<t_field> box(std::unique_ptr<t_field> node) {
    node->set_qualifier(t_field_qualifier::optional);
    node->set_annotation("thrift.box");
    return node;
  }
};

// Standardized field ids.
enum t_op_field_id : t_field_id {
  // ValuePatch
  kAssignId = 1,

  // StructValuePatch
  kClearId = 2,
  kPatchId = 3,
};

// Generate for shared patch fields.
struct ValuePatchGen : StructGen {
  const char* valueName;
  /**
   *  // Assigns to a given struct. If set, all other operations are ignored.
   *  1: optional MyStruct assign (thrift.box);
   */
  std::unique_ptr<t_field> assign(t_type_ref type) {
    return withDoc(
        std::string("Assigns to a given ") + valueName +
            ". If set, all other operations are ignored.\n",
        box(field(kAssignId, type, "assign")));
  }
};

// Generator for a 'StructValuePatch`, see
// patch_generator::add_struct_value_patch.
struct StructValuePatchGen : ValuePatchGen {
  std::unique_ptr<t_field> clear() {
    return withDoc(
        std::string("Clears a given ") + valueName + ". Applied first.\n",
        field(kClearId, t_base_type::t_bool(), "clear"));
  }

  std::unique_ptr<t_field> patch(t_type_ref patch_type) {
    return withDoc(
        std::string("Patches a given ") + valueName + ". Applied second.\n",
        field(kPatchId, patch_type, "patch"));
  }
};

// Generates a patch representation for any struct with the @patch.GeneratePatch
// annotation.
void generate_struct_patch(
    diagnostic_context& ctx, mutator_context& mctx, t_struct& node) {
  auto* annot = node.find_structured_annotation_or_null(kGeneratePatchUri);
  if (annot == nullptr) {
    return;
  }

  // Get the generator for the current program.
  t_program& program = dynamic_cast<t_program&>(*mctx.root());
  auto generator = ctx.cache().get(program, [&]() {
    return std::make_unique<patch_generator>(ctx, program);
  });

  // Add a 'structure patch' and 'struct value patch' using it.
  generator.add_struct_value_patch(
      *annot, node, generator.add_structure_patch(*annot, node));
}

} // namespace

void add_patch_mutators(ast_mutators& mutators) {
  auto& mutator = mutators[standard_mutator_stage::plugin];
  mutator.add_struct_visitor(&generate_struct_patch);
}

t_struct& patch_generator::add_structure_patch(
    const t_node& annot, t_structured& orig) {
  // TODO(afuller): Consider making the name configurable via the annotation.
  auto& generated = gen_struct(annot, orig, "Patch");
  for (const auto& field : orig.fields()) {
    if (auto patch_field = gen_patch_field(annot, field)) {
      ctx_.try_or_failure(
          [&] { generated.append_field(std::move(patch_field)); });
    }
  }
  return generated;
}

t_struct& patch_generator::add_struct_value_patch(
    const t_node& annot, t_struct& orig, t_type_ref patch_type) {
  StructValuePatchGen gen{{{annot}, "struct"}};
  // TODO(afuller): Consider making the name configurable via the annotation.
  auto& generated = gen_struct(annot, orig, "ValuePatch");
  ctx_.try_or_failure([&] { generated.append_field(gen.assign(orig)); });
  ctx_.try_or_failure([&] { generated.append_field(gen.clear()); });
  ctx_.try_or_failure([&] { generated.append_field(gen.patch(patch_type)); });
  return generated;
}

std::unique_ptr<t_field> patch_generator::gen_patch_field(
    const t_node& annot, const t_field& orig) {
  if (t_type_ref patch_type = find_patch_type(*orig.type())) {
    return StructGen{annot}.field(orig.id(), patch_type, orig.name());
  }
  ctx_.warning(orig, "Could not resolve patch type for field.");
  return nullptr;
}

t_type_ref patch_generator::find_patch_type(const t_type& orig) const {
  // Base types use a shared representation defined in patch.thrift.
  //
  // These type should always be availabile because the are defined along side
  // the annoation used to trigger patch generation.
  if (auto* base_type =
          dynamic_cast<const t_base_type*>(orig.get_true_type())) {
    auto itr = patch_types_.find(base_type->base_type());
    if (itr != patch_types_.end()) {
      return itr->second;
    }
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
  program_.add_definition(std::move(generated));
  return *ptr;
}

auto patch_generator::index_patch_types(
    diagnostic_context& ctx, const t_scope& scope) -> patch_type_index {
  patch_type_index index;
  auto add = [&](const std::string& name, t_base_type::type type) {
    if (auto patch_type = scope.find_type(name)) {
      index[type] = *patch_type;
    } else {
      ctx.warning([&](auto& os) {
        os << "Could not find patch type, '" << name << "' for "
           << t_base_type::type_name(type);
      });
    }
  };

  // TODO(afuller): Index all types by uri, and find them that way.
  add("patch.BoolPatch", t_base_type::type::t_bool);
  add("patch.BytePatch", t_base_type::type::t_byte);
  add("patch.I16Patch", t_base_type::type::t_i16);
  add("patch.I32Patch", t_base_type::type::t_i32);
  add("patch.I64Patch", t_base_type::type::t_i64);
  add("patch.FloatPatch", t_base_type::type::t_float);
  add("patch.DoublePatch", t_base_type::type::t_double);
  add("patch.StringPatch", t_base_type::type::t_string);
  add("patch.BinaryPatch", t_base_type::type::t_binary);
  return index;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
