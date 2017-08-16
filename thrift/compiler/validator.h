/*
 * Copyright 2016-present Facebook, Inc.
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

#pragma once

#include <string>
#include <vector>

#include <thrift/compiler/visitor.h>

namespace apache { namespace thrift { namespace compiler {

class validator : virtual public visitor {
 public:
  using errors_t = std::vector<std::string>;
  static errors_t validate(t_program* program);

  using visitor::visit;

  bool visit(t_program* program) override;

 protected:
  void add_error(int lineno, std::string const& message);

 private:
  template <typename T, typename... Args>
  friend std::unique_ptr<T> make_validator(errors_t&, Args&&...);

  void set_ref_errors(errors_t& errors);

  errors_t* errors_{};
  t_program const* program_{};
};

template <typename T, typename... Args>
std::unique_ptr<T> make_validator(
    validator::errors_t& errors, Args&&... args) {
  auto ptr = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  ptr->set_ref_errors(errors);
  return ptr;
}

template <typename T, typename... Args>
validator::errors_t run_validator(t_program* const program, Args&&... args) {
  validator::errors_t errors;
  make_validator<T>(errors, std::forward<Args>(args)...)->traverse(program);
  return errors;
}

class service_method_name_uniqueness_validator : virtual public validator {
 public:
  using validator::visit;

  /**
   * Enforces that there are no duplicate method names either within this
   * service or between this service and any of its ancestors.
   */
  bool visit(t_service* service) override;

 private:
  /**
   * Error messages formatters
   */
  void add_error_service_method_names(
      int lineno,
      std::string const& service_name_new,
      std::string const& service_name_old,
      std::string const& function_name);

  void validate_service_method_names_unique(t_service const* service);
};

class enum_value_names_uniqueness_validator : virtual public validator {
 public:
  using validator::visit;

  // Enforces that there are not duplicated enum value names
  bool visit(t_enum* tenum) override;

 private:
  void validate(t_enum const* tenum);

  void add_validation_error(
      int const lineno,
      std::string const& value_name,
      std::string const& enum_name);
};

class enum_values_uniqueness_validator : virtual public validator {
 public:
  using validator::visit;

  // Enforces that there are not duplicated enum values
  bool visit(t_enum* tenum) override;

 private:
  void validate(t_enum const* tenum);

  void add_validation_error(
      int const lineno,
      t_enum_value const& enum_value,
      std::string const& existing_value_name,
      std::string const& enum_name);
};

class enum_values_set_validator : virtual public validator {
 public:
  using validator::visit;

  // Enforces that every enum value has an explicit value
  bool visit(t_enum* tenum) override;

 private:
  void validate(t_enum const* tenum);

  void add_validation_error(
      int const lineno,
      std::string const& enum_value,
      std::string const& enum_name);
};

class exception_list_is_all_exceptions_validator : virtual public validator {
 public:
  using validator::visit;

  bool visit(t_service* service) override;
};
}}}
