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

#pragma once

#include <string>
#include <unordered_map>

#include <thrift/compiler/ast/diagnostic_context.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/sema/ast_mutator.h>

namespace apache {
namespace thrift {
namespace compiler {

// Adds the patch mutators to the plugin stage of the given mutators.
void add_patch_mutators(ast_mutators& mutators);

// A class that can generate different types of patch representations
// and add them into the given program.
class patch_generator {
 public:
  // Gets the patch_generator for the given context.
  static patch_generator& get_for(
      diagnostic_context& ctx, mutator_context& mctx);

  explicit patch_generator(diagnostic_context& ctx, t_program& program)
      : ctx_(ctx),
        program_(program),
        patch_types_(index_patch_types(ctx, *program.scope())) {}

  // Add a struct with fields 1:1 with the given node, attributing the new node
  // to the given annotation, and return a reference to it.
  //
  // The fields in the generated struct have the same id and name as the
  // original field, but the type is replaced with an associated patch type. For
  // example:
  //
  //   struct BarPatch {
  //     1: FooPatch myFoo;
  //   }
  //
  // would be generated for the struct:
  //
  //   struct Bar {
  //     1: Foo myFoo;
  //   }
  //
  t_struct& add_structure_patch(const t_node& annot, t_structured& node);

  // Add a value patch representation for the given struct and associate patch
  // type, and return a reference to it.
  //
  // The resulting struct has the form:
  //
  //   struct StructValuePatch<Value, Patch> {
  //     // Assigns to a given struct. If set, all other operations are ignored.
  //     optional Value assign (thrift.box);
  //
  //     // Clears a given struct. Applied first.
  //     bool clear;
  //
  //     // Patches a given struct. Applied second.
  //     Patch patch;
  //   }
  //
  t_struct& add_struct_value_patch(
      const t_node& annot, t_struct& value_type, t_type_ref patch_type);

  // Add an optional patch representation for the patch type, and return a
  // reference to it.
  //
  // The resulting struct has the form:
  //
  //   struct OptionalPatch<Value, Patch> {
  //     // Clears a given optional value, leaving it unset. Applied first.
  //     bool clear;
  //
  //     // The patch to apply to any set value. Applied second.
  //     Patch patch;
  //
  //     // The value with which to initialize any unset value. Applied third.
  //     optional Value ensure (thrift.box);
  //
  //     // The patch to apply to any set value, including newly set values.
  //     // Applied forth.
  //     Patch patchAfter;
  //   } (cpp.adapter = "::apache::thrift::op::detail::OptionalPatchAdapter")
  //
  t_struct& add_optional_patch(
      const t_node& annot, t_type_ref value_type, t_struct& patch_type);

 private:
  friend class PatchGeneratorTest;
  using patch_type_index = std::unordered_map<t_base_type::type, t_type_ref>;

  diagnostic_context& ctx_;
  t_program& program_;
  patch_type_index patch_types_;

  // Find and index any patch definitions found in the given scope.
  static patch_type_index index_patch_types(
      diagnostic_context& ctx, const t_scope& scope);

  // Adds a new struct to the program, and return a reference to it.
  t_struct& gen_struct(const t_node& annot, std::string name, std::string uri);
  t_struct& gen_struct_with_suffix(
      const t_node& annot, const t_named& orig, const std::string& suffix);

  // Attempts to resolve the associated patch type for the given field.
  //
  // Returns an empty t_type_ref, if not found.
  t_type_ref find_patch_type(const t_field& field) const;

  // Injects prefix immediately after the last '/'
  //
  // For example: my/path/Name -> my/path/{prefix}Name
  static std::string prefix_uri_name(
      const std::string& uri, const char* prefix) {
    size_t pos = uri.rfind('/') + 1;
    return pos > 0 ? uri.substr(0, pos) + prefix + uri.substr(pos) : "";
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
