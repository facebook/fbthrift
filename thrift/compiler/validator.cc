/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/compiler/validator.h>

#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace apache { namespace thrift { namespace compiler {

std::vector<std::string> validator::validate(t_program const* const program) {
  validator _;
  _.traverse(program);
  return std::move(_).get_errors();
}

std::vector<std::string>&& validator::get_errors() && {
  return std::move(errors_);
}

void validator::add_error(std::string error) {
  errors_.push_back(std::move(error));
}

bool validator::visit(t_program const* const program) {
  program_ = program;
  return true;
}

bool validator::visit(t_service const* const service) {
  validate_service_method_names_unique(service);
  return true;
}

void validator::add_error_service_method_names(
    const std::string& file_path,
    const int lineno,
    const std::string& service_name_new,
    const std::string& service_name_old,
    const std::string& function_name) {
  //[FALIURE:{}:{}] Function {}.{} redefines {}.{}
  std::ostringstream err;
  err << "[FAILURE:" << file_path << ":" << std::to_string(lineno) << "] "
      << "Function " << service_name_new << "." << function_name << " "
      << "redefines " << service_name_old << "." << function_name;
  add_error(err.str());
}

void validator::validate_service_method_names_unique(
  t_service const* const service) {
  // Check for a redefinition of a function in a base service.
  std::unordered_map<std::string, const t_service*> base_function_names;
  for (auto e_s = service->get_extends(); e_s; e_s = e_s->get_extends()) {
    for (const auto& ex_func : e_s->get_functions()) {
      base_function_names[ex_func->get_name()] = e_s;
    }
  }
  for (const auto fnc : service->get_functions()) {
    auto s_pos = base_function_names.find(fnc->get_name());
    const t_service* e_s =
      s_pos != base_function_names.end() ? s_pos->second : nullptr;
    if (e_s) {
      add_error_service_method_names(
          program_->get_path(),
          fnc->get_lineno(),
          service->get_name(),
          e_s->get_full_name(),
          fnc->get_name()
      );
    }
  }

  // Check for a redefinition of a function in the same service.
  std::unordered_set<std::string> function_names;
  for (auto fnc : service->get_functions()) {
    if (function_names.count(fnc->get_name())) {
      add_error_service_method_names(
          program_->get_path(),
          fnc->get_lineno(),
          service->get_name(),
          service->get_name(),
          fnc->get_name()
      );
    }
    function_names.insert(fnc->get_name());
  }
}

}}}
