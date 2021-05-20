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

namespace apache {
namespace thrift {
namespace compiler {

namespace {

void enum_value_name_are_unique(
    diagnostic_results& results, const t_enum* node) {
  std::unordered_set<std::string> names;
  for (const auto* value : node->enum_values()) {
    if (!names.insert(value->name()).second) {
      results.add(
          {diagnostic_level::failure,
           "Redefinition of value `" + value->name() + "` in enum `" +
               node->name() + "`.",
           value,
           node->program()});
    }
  }
}

void enum_values_are_unique(diagnostic_results& results, const t_enum* node) {
  std::unordered_map<int32_t, const t_enum_value*> values;
  for (const auto* value : node->enum_values()) {
    auto prev = values.emplace(value->get_value(), value);
    if (!prev.second) {
      results.add(
          {diagnostic_level::failure,
           "Duplicate value `" + value->name() + "=" +
               std::to_string(value->get_value()) + "` with value `" +
               prev.first->second->name() + "` in enum `" + node->name() + "`.",
           value,
           node->program()});
    }
  }
}

} // namespace

ast_validator standard_validator() {
  ast_validator validator;
  validator.add_enum_visitor(&enum_values_are_unique);
  validator.add_enum_visitor(&enum_value_name_are_unique);
  return validator;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
