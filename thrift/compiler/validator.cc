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

class validator_list {
 public:
  explicit validator_list(std::function<void(validator&)> on_add)
      : on_add_(std::move(on_add)) {}

  std::vector<visitor*> get_pointers() const {
    auto pointers = std::vector<visitor*>{};
    for (auto const& v : validators_) {
      pointers.push_back(v.get());
    }
    return pointers;
  }

  template <typename T, typename... Args>
  void add(Args&&... args) {
    auto ptr = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    on_add_(*ptr);
    validators_.push_back(std::move(ptr));
  }

 private:
  std::function<void(validator&)> on_add_;
  std::vector<std::unique_ptr<validator>> validators_;
};

static void fill_validators(validator_list& vs);

std::vector<std::string> validator::validate(t_program const* const program) {
  auto errors = std::vector<std::string>{};

  auto validators = validator_list([&](validator& v) { v.errors_ = &errors; });
  fill_validators(validators);

  interleaved_visitor(validators.get_pointers()).traverse(program);

  return errors;
}

void validator::add_error(int const lineno, std::string const& message) {
  auto const file = program_->get_path();
  auto const line = std::to_string(lineno);
  std::ostringstream err;
  err << "[FAILURE:" << file << ":" << line << "] " << message;
  errors_->push_back(err.str());
}

bool validator::visit(t_program const* const program) {
  program_ = program;
  return true;
}

/**
 * service_method_name_uniqueness_validator
 */

bool service_method_name_uniqueness_validator::visit(
    t_service const* const service) {
  validate_service_method_names_unique(service);
  return true;
}

void service_method_name_uniqueness_validator::add_error_service_method_names(
    int const lineno,
    std::string const& service_name_new,
    std::string const& service_name_old,
    std::string const& function_name) {
  //[FALIURE:{}:{}] Function {}.{} redefines {}.{}
  std::ostringstream err;
  err << "Function " << service_name_new << "." << function_name << " "
      << "redefines " << service_name_old << "." << function_name;
  add_error(lineno, err.str());
}

void service_method_name_uniqueness_validator::
validate_service_method_names_unique(
    t_service const* const service) {
  // Check for a redefinition of a function in a base service.
  std::unordered_map<std::string, t_service const*> base_function_names;
  for (auto e_s = service->get_extends(); e_s; e_s = e_s->get_extends()) {
    for (auto const ex_func : e_s->get_functions()) {
      base_function_names[ex_func->get_name()] = e_s;
    }
  }
  for (auto const fnc : service->get_functions()) {
    auto const s_pos = base_function_names.find(fnc->get_name());
    auto const e_s =
      s_pos != base_function_names.end() ? s_pos->second : nullptr;
    if (e_s) {
      add_error_service_method_names(
          fnc->get_lineno(),
          service->get_name(),
          e_s->get_full_name(),
          fnc->get_name()
      );
    }
  }

  // Check for a redefinition of a function in the same service.
  std::unordered_set<std::string> function_names;
  for (auto const fnc : service->get_functions()) {
    if (function_names.count(fnc->get_name())) {
      add_error_service_method_names(
          fnc->get_lineno(),
          service->get_name(),
          service->get_name(),
          fnc->get_name()
      );
    }
    function_names.insert(fnc->get_name());
  }
}

/**
 * fill_validators - the validator registry
 *
 * This is where all concrete validator types must be registered.
 */

static void fill_validators(validator_list& vs) {

  vs.add<service_method_name_uniqueness_validator>();
  vs.add<enum_value_names_uniqueness_validator>();

  // add more validators here ...

}

void enum_value_names_uniqueness_validator::add_validation_error(
    int const lineno,
    std::string const& value_name,
    std::string const& enum_name) {
  // [FAILURE:{}] Redefinition of value {} in enum {}
  std::ostringstream err;
  err << "Redefinition of value " << value_name << " in enum " << enum_name;
  add_error(lineno, err.str());
}

bool enum_value_names_uniqueness_validator::visit(t_enum const* const tenum) {
  validate(tenum);
  return true;
}

void enum_value_names_uniqueness_validator::validate(
    t_enum const* const tenum) {
  std::unordered_set<std::string> enum_value_names;
  for (auto v : tenum->get_constants()) {
    if (enum_value_names.count(v->get_name())) {
      add_validation_error(v->get_lineno(), v->get_name(), tenum->get_name());
    }
    enum_value_names.insert(v->get_name());
  }
}
}}}
