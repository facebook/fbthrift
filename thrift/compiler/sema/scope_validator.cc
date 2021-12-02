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

#include <thrift/compiler/sema/scope_validator.h>

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

#include <thrift/compiler/ast/t_exception.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_typedef.h>
#include <thrift/compiler/ast/t_union.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

const std::unordered_map<std::string, std::type_index>& uri_map() {
  static const std::unordered_map<std::string, std::type_index> kUriMap = {
      {"facebook.com/thrift/annotation/Struct", typeid(t_struct)},
      {"facebook.com/thrift/annotation/Union", typeid(t_union)},
      {"facebook.com/thrift/annotation/Exception", typeid(t_exception)},
      {"facebook.com/thrift/annotation/Field", typeid(t_field)},
      {"facebook.com/thrift/annotation/Typedef", typeid(t_typedef)},
      {"facebook.com/thrift/annotation/Service", typeid(t_service)},
      {"facebook.com/thrift/annotation/Interaction", typeid(t_interaction)},
      {"facebook.com/thrift/annotation/Function", typeid(t_function)},
      {"facebook.com/thrift/annotation/Enum", typeid(t_enum)},
      {"facebook.com/thrift/annotation/EnumValue", typeid(t_enum_value)},
      {"facebook.com/thrift/annotation/Const", typeid(t_const)},
  };
  return kUriMap;
}

struct allowed_scopes {
  std::unordered_set<std::type_index> types;

  explicit allowed_scopes(node_metadata_cache& cache, const t_const& annot) {
    for (const auto* meta_annot : annot.get_type()->structured_annotations()) {
      const t_type& meta_annot_type = *meta_annot->get_type();
      if (is_transitive_annotation(meta_annot_type)) {
        const auto& transitive_scopes =
            cache.get<allowed_scopes>(*meta_annot).types;
        types.insert(transitive_scopes.begin(), transitive_scopes.end());
      }
      const auto& uri = meta_annot->get_type()->get_annotation("thrift.uri");
      auto itr = uri_map().find(uri);
      if (itr != uri_map().end()) {
        types.emplace(itr->second);
      }
    }
  }
};

} // namespace

void validate_annotation_scopes(diagnostic_context& ctx, const t_named& node) {
  // Ignore a transitive annotation definition because it is a collection of
  // annotations that apply at other scopes. For example:
  //
  //   @cpp.Ref{type = cpp.RefType.Unique}
  //   @meta.Transitive
  //   struct MyAnnotation {}
  //
  // Although @cpp.Ref is a field annotation we don't emit a diagnostic here
  // because it applies not to the definition of MyAnnotation but to its uses.
  if (is_transitive_annotation(node)) {
    return;
  }

  for (const t_const* annot : node.structured_annotations()) {
    const t_type* annot_type = &*annot->type();
    // Ignore scoping annotations themselves.
    if (uri_map().find(annot_type->get_annotation("thrift.uri")) !=
        uri_map().end()) {
      continue;
    }

    // Get the allowed set of node types.
    const auto& allowed = ctx.cache().get<allowed_scopes>(*annot);

    if (allowed.types.empty()) {
      // Warn that the annotation isn't marked as such.
      ctx.warning_strict(*annot, [&](auto& o) {
        o << "Using `" << annot_type->name()
          << "` as an annotation, even though it has not been enabled for any "
             "annotation scope.";
      });
    } else if (allowed.types.find(typeid(node)) == allowed.types.end()) {
      // Type mismatch.
      ctx.failure(*annot, [&](auto& o) {
        o << "`" << annot_type->name() << "` cannot annotate `" << node.name()
          << "`";
      });
    }
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
