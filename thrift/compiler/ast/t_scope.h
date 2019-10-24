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

#ifndef T_SCOPE_H
#define T_SCOPE_H

#include <map>
#include <set>

#include <thrift/compiler/ast/t_const.h>
#include <thrift/compiler/ast/t_enum.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * This represents a variable scope used for looking up predefined types and
 * services. Typically, a scope is associated with a t_program. Scopes are not
 * used to determine code generation, but rather to resolve identifiers at
 * parse time.
 *
 */
class t_scope {
 public:
  t_scope() {}

  void add_type(std::string name, t_type* type) {
    types_[name] = type;
  }

  t_type* get_type(std::string name) {
    return types_[name];
  }

  void add_service(std::string name, t_service* service) {
    services_[name] = service;
  }

  t_service* get_service(std::string name) {
    return services_[name];
  }

  void add_constant(std::string name, t_const* constant);

  std::vector<std::string> split_string_by_periods(std::string str);

  std::string join_strings_by_commas(std::set<std::string> strs);

  t_const* get_constant(std::string name) {
    return constants_[name];
  }

  bool is_ambiguous_enum_value(std::string enum_value_name) {
    return redefined_enum_values_.find(enum_value_name) !=
        redefined_enum_values_.end();
  }

  std::string get_fully_qualified_enum_value_names(std::string name);

  void print() {
    std::map<std::string, t_type*>::iterator iter;
    for (iter = types_.begin(); iter != types_.end(); ++iter) {
      printf(
          "%s => %s\n", iter->first.c_str(), iter->second->get_name().c_str());
    }
  }

 private:
  // Map of names to types
  std::map<std::string, t_type*> types_;

  // Map of names to constants
  std::map<std::string, t_const*> constants_;

  // Map of names to services
  std::map<std::string, t_service*> services_;

  // Set of enum_values that are redefined and are ambiguous
  // if referred to without the enum name
  std::set<std::string> redefined_enum_values_;

  // Map of enum_values to their definition full names
  std::map<std::string, std::set<std::string>> enum_values_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache

#endif
