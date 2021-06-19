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

  explicit allowed_scopes(const t_const& annot) {
    for (const auto* meta_annot : annot.get_type()->structured_annotations()) {
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
  for (const auto* annot : node.structured_annotations()) {
    // Ignore scoping annotations themselves.
    if (uri_map().find(annot->get_type()->get_annotation("thrift.uri")) !=
        uri_map().end()) {
      continue;
    }

    // Get the allowed set of node types.
    const auto& allowed = ctx.cache().get<allowed_scopes>(*annot);

    if (allowed.types.empty()) {
      // Warn that the annotation isn't marked as such.
      ctx.warning_strict(*annot, [&](auto& o) {
        o << "Using `" << annot->get_type()->name()
          << "` as an annotation, even though it has not been enabled for any annotation scope.";
      });
    } else if (allowed.types.find(typeid(node)) == allowed.types.end()) {
      // Type mismatch.
      ctx.failure(*annot, [&](auto& o) {
        o << "`" << annot->get_type()->name() << "` cannot annotate `"
          << node.name() << "`";
      });
    }
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
