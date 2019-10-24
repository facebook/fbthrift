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

#include <istream>
#include <iterator>
#include <ostream>

#include <thrift/compiler/ast/t_enum_value.h>
#include <thrift/compiler/ast/t_scope.h>

namespace apache {
namespace thrift {
namespace compiler {

std::vector<std::string> t_scope::split_string_by_periods(std::string str) {
  std::istringstream iss(str);
  std::vector<std::string> tokens;
  std::string token;
  while (std::getline(iss, token, '.')) {
    if (!token.empty()) {
      tokens.push_back(token);
    }
  }
  return tokens;
}

std::string t_scope::join_strings_by_commas(std::set<std::string> strs) {
  std::ostringstream stream;
  std::copy(
      strs.begin(),
      strs.end(),
      std::ostream_iterator<std::string>(stream, ", "));
  std::string joined_str = stream.str();
  // Remove last comma
  return joined_str.empty() ? joined_str
                            : joined_str.substr(0, joined_str.size() - 2);
}

void t_scope::add_constant(std::string name, t_const* constant) {
  if (constant && constant->get_value()->is_enum()) {
    const std::string& enum_value_name =
        constant->get_value()->get_enum_value()->get_name();
    std::vector<std::string> name_split =
        t_scope::split_string_by_periods(name);
    if (enum_value_name.compare("UNKNOWN") &&
        (constants_.find(name) != constants_.end()) && constants_[name]) {
      redefined_enum_values_.insert(name);
    }
    if (name_split.size() == 3) {
      auto name_with_enum = name_split[1] + '.' + name_split[2];
      enum_values_[enum_value_name].insert(name_with_enum);
    }
  }
  constants_[name] = constant;
}

std::string t_scope::get_fully_qualified_enum_value_names(std::string name) {
  // Get just the enum value name from name, which is
  // PROGRAM_NAME.ENUM_VALUE_NAME
  auto name_split = split_string_by_periods(name);
  if (name_split.empty()) {
    return "";
  }
  return join_strings_by_commas(
      enum_values_[name_split[name_split.size() - 1]]);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
